#include "../include/engine_sim_application.h"

#include <iostream>
#include <codecvt>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)nCmdShow;
    (void)lpCmdLine;
    (void)hPrevInstance;

    /*
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    */

    std::string comPort = "COM3";

    LPWSTR* szArglist;
    int nArgs;
    int i;

    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (NULL == szArglist)
    {
        wprintf(L"CommandLineToArgvW failed\n");
        return 0;
    }
    else {
        for (i = 0; i < nArgs; i++) {
            std::wstring s(szArglist[i]);
            if (s._Starts_with(L"COM")) {
                using convert_typeX = std::codecvt_utf8<wchar_t>;
                std::wstring_convert<convert_typeX, wchar_t> converterX;
                comPort = converterX.to_bytes(s);
            }
            printf("%d: %ws\n", i, szArglist[i]);
        }
    }

    EngineSimApplication application(comPort);
    application.initialize((void *)&hInstance, ysContextObject::DeviceAPI::DirectX11);
    application.run();
    application.destroy();

    return 0;
}
