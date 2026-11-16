/*
 * namedpipe.c - Stage 08, exercise E: a named-pipe server, and who is on
 * the other end.
 *
 * Build:  gcc -O2 -Wall -o namedpipe.exe namedpipe.c -ladvapi32
 * Run:    namedpipe.exe          (starts a copy of itself as the client)
 *
 * 1. The server creates \\.\pipe\lab08-namedpipe-<pid> in message mode,
 *    with FILE_FLAG_FIRST_PIPE_INSTANCE (no one else may already own the
 *    name) and PIPE_REJECT_REMOTE_CLIENTS, and starts its client.
 * 2. Who connected: GetNamedPipeClientProcessId, compared with the PID that
 *    CreateProcess returned, and the client's user and integrity level,
 *    read from its token while impersonating it at identification level.
 *    That is the NT counterpart of SO_PEERCRED. The client checks the
 *    server the same way with GetNamedPipeServerProcessId.
 * 3. Message mode keeps boundaries: a 100-byte message read into a 64-byte
 *    buffer arrives in two parts, the first with ERROR_MORE_DATA.
 * 4. Name squatting: a second CreateNamedPipe for the same name with
 *    FILE_FLAG_FIRST_PIPE_INSTANCE fails; without the flag it succeeds.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static void die(const char *what)
{
    printf("%s failed: error %lu\n", what, GetLastError());
    exit(1);
}

static const char *integrity_name(DWORD rid)
{
    if (rid >= SECURITY_MANDATORY_SYSTEM_RID)
        return "system";
    if (rid >= SECURITY_MANDATORY_HIGH_RID)
        return "high";
    if (rid >= SECURITY_MANDATORY_MEDIUM_RID)
        return "medium";
    if (rid >= SECURITY_MANDATORY_LOW_RID)
        return "low";
    return "untrusted";
}

/* Print the user and integrity level of a token; return a copy of its SID. */
static PSID describe_token(const char *who, HANDLE tok)
{
    BYTE ubuf[256], ibuf[256];
    DWORD len;
    if (!GetTokenInformation(tok, TokenUser, ubuf, sizeof ubuf, &len))
        die("GetTokenInformation(TokenUser)");
    PSID sid = ((TOKEN_USER *)ubuf)->User.Sid;
    LPWSTR ssid = NULL;
    ConvertSidToStringSidW(sid, &ssid);
    wchar_t name[128], domain[128];
    DWORD nlen = 128, dlen = 128;
    SID_NAME_USE use;
    if (!LookupAccountSidW(NULL, sid, name, &nlen, domain, &dlen, &use))
        wcscpy(name, L"?"), wcscpy(domain, L"?");
    DWORD rid = 0;
    if (GetTokenInformation(tok, TokenIntegrityLevel, ibuf, sizeof ibuf, &len)) {
        PSID il = ((TOKEN_MANDATORY_LABEL *)ibuf)->Label.Sid;
        rid = *GetSidSubAuthority(il, *GetSidSubAuthorityCount(il) - 1);
    }
    printf("  %s: user %ls\\%ls (%ls), integrity %s (0x%lx)\n", who, domain, name, ssid,
           integrity_name(rid), rid);
    LocalFree(ssid);
    DWORD n = GetLengthSid(sid);
    PSID copy = malloc(n);
    CopySid(n, copy, sid);
    return copy;
}

static int client(const wchar_t *name)
{
    /* SECURITY_IDENTIFICATION: the server may read our token but may not
     * act as us. Without SECURITY_SQOS_PRESENT the default level is
     * SecurityImpersonation. */
    HANDLE h = CreateFileW(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                           SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, NULL);
    if (h == INVALID_HANDLE_VALUE)
        die("client CreateFile");
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(h, &mode, NULL, NULL);
    ULONG spid = 0;
    GetNamedPipeServerProcessId(h, &spid);
    char big[3000];
    memset(big, 'x', sizeof big);
    struct {
        const void *p;
        DWORD n;
    } msgs[] = {{"hello from the client", 21}, {big, 100}, {big, 3000}};
    for (int i = 0; i < 3; i++) {
        DWORD w;
        if (!WriteFile(h, msgs[i].p, msgs[i].n, &w, NULL))
            die("client WriteFile");
    }
    char reply[64];
    DWORD r = 0;
    BOOL ok = ReadFile(h, reply, sizeof reply - 1, &r, NULL);
    reply[ok ? r : 0] = 0;
    /* print only now, so the two processes' lines do not interleave */
    printf("  client: pid %lu; GetNamedPipeServerProcessId says the server is pid %lu\n",
           GetCurrentProcessId(), spid);
    printf("  client: sent 3 messages (21, 100, 3000 bytes); server replied \"%s\"\n", reply);
    CloseHandle(h);
    return 0;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc >= 3 && strcmp(argv[1], "client") == 0) {
        wchar_t name[128];
        swprintf(name, 128, L"%hs", argv[2]);
        return client(name);
    }

    wchar_t name[128];
    char aname[128];
    snprintf(aname, sizeof aname, "\\\\.\\pipe\\lab08-namedpipe-%lu", GetCurrentProcessId());
    swprintf(name, 128, L"%hs", aname);

    printf("1. A message-mode pipe and its client\n");
    HANDLE h = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT |
                                    PIPE_REJECT_REMOTE_CLIENTS,
                                PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        die("CreateNamedPipe");
    printf("  server: pid %lu created %ls\n", GetCurrentProcessId(), name);

    wchar_t self[MAX_PATH], cmd[512];
    GetModuleFileNameW(NULL, self, MAX_PATH);
    swprintf(cmd, 512, L"\"%ls\" client %hs", self, aname);
    STARTUPINFOW si = {.cb = sizeof si};
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        die("CreateProcess");
    printf("  server: started the client, CreateProcess says pid %lu\n", pi.dwProcessId);

    if (!ConnectNamedPipe(h, NULL) && GetLastError() != ERROR_PIPE_CONNECTED)
        die("ConnectNamedPipe");

    printf("\n2. Who connected?\n");
    ULONG cpid = 0, csess = 0, mysess = 0;
    GetNamedPipeClientProcessId(h, &cpid);
    GetNamedPipeClientSessionId(h, &csess);
    ProcessIdToSessionId(GetCurrentProcessId(), &mysess);
    printf("  server: GetNamedPipeClientProcessId %lu (%s CreateProcess), session %lu (mine %lu)\n",
           cpid, cpid == pi.dwProcessId ? "matches" : "differs from", csess, mysess);

    char buf[64];
    DWORD got = 0;
    if (!ReadFile(h, buf, sizeof buf - 1, &got, NULL))
        die("ReadFile");
    buf[got] = 0;
    printf("  server: first message: \"%s\" (%lu bytes)\n", buf, got);

    PSID client_sid = NULL, my_sid;
    if (!ImpersonateNamedPipeClient(h))
        die("ImpersonateNamedPipeClient");
    HANDLE tok;
    /* Take the client's token handle, then stop impersonating at once; the
     * handle stays valid, and account lookups run as the server again. */
    BOOL have_tok = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &tok);
    DWORD tok_err = GetLastError();
    RevertToSelf();
    if (have_tok) {
        SECURITY_IMPERSONATION_LEVEL lvl;
        DWORD len;
        GetTokenInformation(tok, TokenImpersonationLevel, &lvl, sizeof lvl, &len);
        printf("  server: impersonated the client at level %s and took its token\n",
               lvl == SecurityIdentification  ? "SecurityIdentification"
               : lvl == SecurityImpersonation ? "SecurityImpersonation"
                                              : "other");
        client_sid = describe_token("client's token", tok);
        CloseHandle(tok);
    } else {
        printf("  server: OpenThreadToken: error %lu\n", tok_err);
    }
    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok);
    my_sid = describe_token("server's token", tok);
    CloseHandle(tok);
    int same = client_sid && EqualSid(client_sid, my_sid);
    printf("  server: same user: %s -> %s\n", same ? "yes" : "no", same ? "accept" : "reject");

    printf("\n3. Message boundaries\n");
    for (int i = 0; i < 2; i++) {
        DWORD avail = 0, left = 0;
        PeekNamedPipe(h, NULL, 0, NULL, &avail, &left);
        printf("  server: next message is %lu bytes (%lu bytes in the pipe in total)\n", left,
               avail);
        int more = 0;
        for (;;) {
            BOOL ok = ReadFile(h, buf, sizeof buf, &got, NULL);
            DWORD e = ok ? 0 : GetLastError();
            if (!ok && e == ERROR_MORE_DATA) {
                more++;
                continue;
            }
            printf("    ReadFile into 64 bytes: %d x (64 bytes, FALSE, ERROR_MORE_DATA), then "
                   "%lu bytes, %s\n",
                   more, got, ok ? "TRUE: end of message" : "an error");
            break;
        }
    }

    printf("\n4. Name squatting\n");
    HANDLE h2 = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                 PIPE_TYPE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, NULL);
    printf("  a second instance with FILE_FLAG_FIRST_PIPE_INSTANCE: %s, error %lu\n",
           h2 == INVALID_HANDLE_VALUE ? "failed" : "created", GetLastError());
    if (h2 != INVALID_HANDLE_VALUE)
        CloseHandle(h2);
    HANDLE h3 = CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE,
                                 PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, NULL);
    printf("  a second instance without the flag:           %s\n",
           h3 == INVALID_HANDLE_VALUE ? "failed"
                                      : "created (the flag guards only the first instance)");
    if (h3 != INVALID_HANDLE_VALUE)
        CloseHandle(h3);

    DWORD w;
    WriteFile(h, same ? "accepted" : "rejected", 9, &w, NULL);
    FlushFileBuffers(h);
    WaitForSingleObject(pi.hProcess, 10000);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    printf("\n  server: client exited with code %lu\n", code);
    DisconnectNamedPipe(h);
    CloseHandle(h);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
