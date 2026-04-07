#if defined(linux) || defined(__linux) || defined(__linux__)
#include <cstdint>
#include <cstdio>
#include <cstring>
#elif defined(_WIN32)
#include <Windows.h>
#include <shobjidl.h>
#endif

#include <string>
#include "utils/SimpleFileDialog.h"



std::string cr::utils::SimpleFileDialog::dialog()
{
#if defined(linux) || defined(__linux) || defined(__linux__)
    char filename[1024] = {};

    FILE* f = popen("zenity --file-selection", "r");
    if (!f) return "";

    if (!fgets(filename, sizeof(filename), f)) {
        pclose(f);
        return "";
    }

    pclose(f);

    size_t len = strlen(filename);
    if (len > 0 && filename[len - 1] == '\n') {
        filename[len - 1] = '\0';
    }

    return std::string(filename);

#elif defined(_WIN32)
    IFileOpenDialog* pFileOpen = nullptr;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return "";
    }

    std::string result;

    hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        NULL,
        CLSCTX_ALL,
        IID_IFileOpenDialog,
        (void**)&pFileOpen
    );

    if (SUCCEEDED(hr) && pFileOpen) {
        pFileOpen->SetTitle(L"OPEN VIDEO FILE");

        hr = pFileOpen->Show(NULL);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileOpen->GetResult(&pItem);

            if (SUCCEEDED(hr) && pItem) {
                PWSTR file = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &file);

                if (SUCCEEDED(hr) && file) {
                    int count = WideCharToMultiByte(CP_UTF8, 0, file, -1, NULL, 0, NULL, NULL);
                    if (count > 0) {
                        std::string utf8(count - 1, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, file, -1, utf8.data(), count, NULL, NULL);
                        result = utf8;
                    }
                    CoTaskMemFree(file);
                }

                pItem->Release();
            }
        }

        pFileOpen->Release();
    }

    CoUninitialize();
    return result;

#else
    return "";
#endif
}
