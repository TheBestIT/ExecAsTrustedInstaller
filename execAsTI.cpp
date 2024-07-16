#include <tchar.h>
#include <Windows.h>
#include <iostream>

LPQUERY_SERVICE_CONFIG lpsc = nullptr;
LPSERVICE_DESCRIPTION lpsd = nullptr;
DWORD dwBytesNeeded = 0, cbBufSize = 0, dwError = 0;
SC_HANDLE hSCManager;
SC_HANDLE hTrustedInstaller;

int stage = 0;

void cleanAll() {
    if (stage > 0) CloseServiceHandle(hSCManager);  
    if (stage > 1) CloseServiceHandle(hTrustedInstaller);
    if (stage > 2) LocalFree(lpsc);
    if (stage = 4) LocalFree(lpsd);
}

BOOL IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof( TOKEN_ELEVATION );
        if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if( hToken ) {
        CloseHandle(hToken);
    }
    return fRet;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: execAsTI.exe <\"command\">" << std::endl;
        return 1;
    }

    if (!IsElevated()) {
        std::cout << "This Program must be executed as Administrator" << std::endl;
        return 1;
    }

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Succesfully got handle to Service Manager" << std::endl;

    stage++;
    hTrustedInstaller = OpenService(hSCManager, _T("TrustedInstaller"), SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_START);
    if (hTrustedInstaller == NULL) {
        std::cerr << "OpenService failed: " << GetLastError() << std::endl;
        cleanAll();
        return 1;
    }

    std::cout << "Succesfully got handle to TrustedInstaller Service\n" << std::endl;

    stage++;
    // Mock Query request to get required allocating space for the real query rq
    if (!QueryServiceConfig(hTrustedInstaller, NULL, 0, &dwBytesNeeded)) { 
        dwError = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == dwError) {
            cbBufSize = dwBytesNeeded;
            lpsc = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, cbBufSize); // Alloc required space for real query
            if (!lpsc) {
                std::cerr << "LocalAlloc failed for lpsc: " << GetLastError() << std::endl;
                cleanAll();
                return 1;
            }
            std::cout << "Triggered QueryServiceConfig:ERROR_INSUFFICIENT_BUFFER as expected" << std::endl;
        } else {
            std::cerr << "QueryServiceConfig failed: " << dwError << std::endl;
            cleanAll();
            return 1;
        }
    }

    std::cout << "Got buffer size for current service config" << std::endl;

    stage++;
    // Get reference to config
    if (!QueryServiceConfig(hTrustedInstaller, lpsc, cbBufSize, &dwBytesNeeded)) {
        std::cerr << "QueryServiceConfig failed on second call: " << GetLastError() << std::endl;
        cleanAll();
        return 1;
    }

    stage++;
    std::cout << "Saved TrustedInstaller service current config\n" << std::endl;

    std::string strInjectedCommand = "cmd.exe /c ";
    strInjectedCommand += argv[1];
    LPCSTR injectedCommand = strInjectedCommand.c_str();

    std::cout << "Injecting '" << strInjectedCommand << "' into TrustedInstaller..." << std::endl;

    if (!ChangeServiceConfigA(
        hTrustedInstaller,
        SERVICE_NO_CHANGE,
        SERVICE_AUTO_START, // Start Type
        SERVICE_NO_CHANGE,
        injectedCommand, // BinPath
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL)) {
            std::cerr << "ChangeServiceConfig failed: " << GetLastError() << std::endl;
            cleanAll();
            return 1;
    }

    std::cout << "Service configurations updated successfully.\n" << std::endl;
    std::cout << "Starting Service..." << std::endl;

    StartService(hTrustedInstaller, 0, NULL);

    std::cout << "Injected code executed. Reverting changes made to service config...\n" << std::endl;

    if (!ChangeServiceConfig(
        hTrustedInstaller,
        SERVICE_NO_CHANGE,
        lpsc->dwStartType, // Start Type
        SERVICE_NO_CHANGE,
        lpsc->lpBinaryPathName, // BinPath
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL)) {
            std::cerr << "ChangeServiceConfig failed: " << GetLastError() << std::endl;
            cleanAll();
            return 1;
    }

    std::cout << "All done!" << std::endl;

    cleanAll();
    return 0;
}