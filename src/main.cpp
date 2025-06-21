#include <iostream>
#include <string>

//microsoft, wtf.
//is it this complicated to handle audio?
#include <comdef.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h> // Required for IAudioEndpointVolume
#include <Functiondiscoverykeys_devpkey.h> // Include this for PKEY_Device_FriendlyName
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include "Mmdeviceapi.h"
#include "PolicyConfig.h"
#include "Propidl.h"
#include "Functiondiscoverykeys_devpkey.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "kernel32.lib") // For FormatMessage
#pragma comment(lib, "user32.lib")



//most of the complexity winapi has isn't necessary. hide the complexity here and fix the godawful naming scheme
class AudioDevice {
private:
    IMMDevice* pDevice;
    IAudioEndpointVolume* pAudioEndpointVolume;
public:
    AudioDevice(IMMDevice* pDevice) : pDevice(pDevice), pAudioEndpointVolume(nullptr) {
        if (this->pDevice) {
            this->pDevice->AddRef();
            HRESULT hr = this->pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&this->pAudioEndpointVolume);
            if (FAILED(hr)) {
                std::cerr << "Activate(IAudioEndpointVolume) failed: 0x" << std::hex << hr << std::endl;
                this->pDevice->Release();
                throw std::runtime_error("Failed to activate audio endpoint volume");
            }
        }
    }
    AudioDevice(nullptr_t null) : pDevice(nullptr), pAudioEndpointVolume(nullptr) {}

    // Copy constructor
    AudioDevice(const AudioDevice& other) : pDevice(other.pDevice), pAudioEndpointVolume(nullptr) {
        if (pDevice) {
            pDevice->AddRef();
            HRESULT hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&this->pAudioEndpointVolume);
            if (FAILED(hr)) {
                std::cerr << "Activate (copy) failed: 0x" << std::hex << hr << std::endl;
                pDevice->Release();
                this->pDevice = nullptr;
               throw std::runtime_error("Failed copy operation on audio device");
            }
        }
    }

    // Assignment operator
    AudioDevice& operator=(const AudioDevice& other) { // why do i have to define = :(
        if (this != &other) {
            // Release existing resources
            if (pAudioEndpointVolume) {
                pAudioEndpointVolume->Release();
                pAudioEndpointVolume = nullptr;
            }
            if (pDevice) {
                pDevice->Release();
                pDevice = nullptr;
            }

            // Copy from the other
            pDevice = other.pDevice;
            if (pDevice) {
                pDevice->AddRef();
                HRESULT hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&this->pAudioEndpointVolume);
                if (FAILED(hr)) {
                    std::cerr << "Activate (assign) failed: 0x" << std::hex << hr << std::endl;
                    pDevice->Release();
                    pDevice = nullptr;
                    pAudioEndpointVolume = nullptr;
                    // Handle failure
                }
            }
        }
        return *this;
    }

    ~AudioDevice() {
        if (pAudioEndpointVolume) {
            pAudioEndpointVolume->Release();
            pAudioEndpointVolume = nullptr;
        }
        if (pDevice) {
            pDevice->Release();
            pDevice = nullptr;
        }
    }

        /*
        * Sets the volume of the audio device
        * @param level must be between 0 and 1
        * @return bool representing success
        */
        bool SetVolume(float level) {
            if (level < 0.0 || level > 1.0) {
                throw std::invalid_argument("Volume must be between 0.0 and 1.0"); //should this complain loudly or silently?
            }
            IAudioEndpointVolume* pAudioEndpointVolume = nullptr;
            HRESULT hr = this->pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pAudioEndpointVolume);
            if (FAILED(hr)) {
                std::cerr << "Activate(IAudioEndpointVolume) failed: 0x" << std::hex << hr << std::endl;
                return false;
            }
            pAudioEndpointVolume->SetMasterVolumeLevelScalar(level, nullptr);
            pAudioEndpointVolume->Release();
            return true;
        }
        float Volume() {
            IAudioEndpointVolume* pAudioEndpointVolume = nullptr;
            HRESULT hr = this->pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pAudioEndpointVolume);
            if (FAILED(hr)) {
                std::cerr << "Activate(IAudioEndpointVolume) failed: 0x" << std::hex << hr << std::endl;
                return -1;
            }
            float volume;
            pAudioEndpointVolume->GetMasterVolumeLevelScalar(&volume);
            pAudioEndpointVolume->Release();
            return volume;
        }
        std::wstring Name() {
            IPropertyStore* pProps = nullptr;
            LPWSTR pwszName = nullptr;
            std::wstring friendlyName;
            HRESULT hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if (SUCCEEDED(hr)) {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                if (SUCCEEDED(hr) && varName.vt == VT_LPWSTR) {
                    friendlyName = varName.pwszVal;
                }
                PropVariantClear(&varName);
                pProps->Release();
            }
            return friendlyName;
        }
        bool Mute(bool mute) {
            if (mute == false) {
                std::wcout << "Unmuted:";
            } else {
                std::wcout << "Muted: ";
            }
            std::wcout << this->Name() << std::endl;
            HRESULT hr = this->pAudioEndpointVolume->SetMute((BOOL)mute, nullptr);
            return SUCCEEDED(hr);
        }
        bool Muted() {
            BOOL muted;
            HRESULT hr = pAudioEndpointVolume->GetMute(&muted);
            return muted;
        }
        bool Active() {
            if (this->pDevice == nullptr) {
                return false; //this should never happen
            }
            DWORD state;
            HRESULT hr = this->pDevice->GetState(&state);
            if (SUCCEEDED(hr) && state == DEVICE_STATE_ACTIVE) { //can control the device
                    return true;
            }
            return false;
        }
        bool setActive() {
            LPWSTR id = nullptr;
            this->pDevice->GetId(&id);
            IPolicyConfigVista *pPolicyConfig;
            ERole reserved = eConsole;
            HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
            if (SUCCEEDED(hr))
                {
                    hr = pPolicyConfig->SetDefaultEndpoint(id, reserved);
                    pPolicyConfig->Release();
                }
            return hr;
        }
};

class AudioDeviceList {
    private: 
        IMMDeviceEnumerator* pEnumerator;
        IMMDeviceCollection* pDeviceCollection;
    public:
        AudioDeviceList() {
            this->pEnumerator = nullptr;            
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&this->pEnumerator);
            if (FAILED(hr)) {
                std::cerr << "CoCreateInstance failed: 0x" << std::hex << hr << std::endl;
            }
            this->pDeviceCollection = nullptr;
            hr = this->pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &this->pDeviceCollection);
            if (FAILED(hr)) {
                std::cerr << "EnumAudioEndpoints failed: 0x" << std::hex << hr << std::endl;
            }

        }
        ~AudioDeviceList() {
            this->pEnumerator->Release();
        }
        AudioDevice DefaultSpeaker() {
            IMMDevice* pDefaultRenderDevice = nullptr;
            HRESULT hr = this->pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultRenderDevice);
            if (FAILED(hr)) {
                std::cerr << "GetDefaultAudioEndpoint failed: 0x" << std::hex << hr << std::endl;
            }
            return AudioDevice(pDefaultRenderDevice);
        }
        AudioDevice Get(unsigned int index) {
            unsigned int numDevices = 0;
            pDeviceCollection->GetCount(&numDevices);
            if (index < 0 || index >= numDevices) {
                throw std::invalid_argument("Index out of range");
            }
            IMMDevice* pDevice = nullptr;
            HRESULT hr = pDeviceCollection->Item(index, &pDevice);
            if (FAILED(hr)) {
                std::cerr << "Item failed: 0x" << std::hex << hr << std::endl;
            }
            return AudioDevice(pDevice);
        }
        AudioDevice Get(std::wstring name) { //bluetooth device names allow UTF-8
            IMMDevice* pDevice = nullptr;
            std::vector<AudioDevice> list = this->toVec(); 
            for (AudioDevice device: list) {
                if (device.Name().find(name) != std::wstring::npos) {
                    return device;
                }
            }
            throw std::invalid_argument("Device not found");
        }
        int Length() {
            unsigned int numDevices = 0;
            pDeviceCollection->GetCount(&numDevices);
            return numDevices;
        }
        std::vector<AudioDevice> toVec() {
            int len = this->Length();
            std::vector<AudioDevice> list; 
            for(int i = 0;i < len; i++) {
                list.push_back(this->Get(i));
            } 
            std::cout << "Found " << len << " devices" << std::endl;
            return list;
        }
};

AudioDevice getAudioDevice(char* device) {
    AudioDeviceList audioDevices;
    if (isdigit(device[0])) {
        return audioDevices.Get(atoi(device));
    }
    if (device == nullptr) {
            try{
            return audioDevices.DefaultSpeaker();
            } catch(...) {
                std::cerr << "Error: No default speaker found. do you have a speaker connected?" << std::endl;
                throw;
            }
    }
    //convert char* to wstring
    std::wstring name(device, device + strlen(device));
    return audioDevices.Get(name);
}
bool caselessstrcmp(const char* str1, const char* str2) {
    if (strlen(str1) != strlen(str2)) {
        return false;
    }
    for (int i = 0; i < strlen(str1); i++) {
        if (tolower(str1[i]) != tolower(str2[i])) {
            return false;
        }
    }
    return true;
}

std::optional<bool> strtobool(char* str) { //true, false, maybe
    std::vector<std::string> truthy = {"true", "yes", "on", "y", "t"};
    std::vector<std::string> falsy = {"false", "no", "off", "n", "f"};
    for (std::string& s : truthy) {
        if (caselessstrcmp(str, s.c_str())) {
            return true;
        }
    }
    for (std::string& s : falsy) {
        if (caselessstrcmp(str, s.c_str())) {
            return false;
        }
    }
    if (!isdigit(str[0])) {
        return std::nullopt;
    }
    return bool(strtol(str, nullptr, 10));
}
bool mute(int argc, char* argv[], AudioDevice& speaker) {
    if (&speaker == nullptr) {
        return false;
    }
    bool willMute = strtobool(argv[argc-1]).value_or(!speaker.Muted());
    return speaker.Mute(willMute);
}
bool volume(int argc, char* argv[], AudioDevice& speaker) {
    if (&speaker == nullptr) {
        return false;
    }

    float newVolume;

    //handle relative volume
    if (argv[argc-1][0] == '+'|| argv[argc-1][0] == '-') {
        newVolume = float(atof(argv[argc-1]+1));
        if (argv[argc-1][0] == '+') {
            newVolume = speaker.Volume() + newVolume;
        }
        if(argv[argc-1][0]=='-') {
            newVolume =speaker.Volume() - newVolume;
        }
    } else { //absolute volume
        try {
            newVolume = float(atof(argv[argc-1])); //TODO: check if atof succeeded
        } catch(...) { //fed invalid volume argument
            return false;
        }
    }
    //clamp to valid range
    return speaker.SetVolume(std::clamp(newVolume, 0.0f, 1.0f));
}
bool list(int argc, char* argv[], AudioDevice& speaker) {
    AudioDeviceList audioDevices;
    std::vector<AudioDevice> list = audioDevices.toVec();
    for (int idx = 0; AudioDevice device: list) {
        std::wcout << idx << ": " << device.Name() << std::endl;
        idx++;
    }
    return true;
}
bool setdefault(int argc, char* argv[], AudioDevice& speaker) {
    HRESULT hr = speaker.setActive();
    volume(argc-1, argv+1, speaker);
    return SUCCEEDED(hr);
}
bool help(int argc, char* argv[], AudioDevice& speaker) {
    std::wcout << "Usage: " << argv[0] << " [device] [verb]" << std::endl;
    std::wcout << "Verbs:" << std::endl;
    std::wcout << "\tmute t[rue]/f[alse]" << std::endl;
    std::wcout << "\tvolume [+-]0.0-1.0" << std::endl;
    std::wcout << "\tlist : lists devices" << std::endl;
    std::wcout << "\tsetdefault : sets the default device" << std::endl;
    std::wcout << "\thelp : displays this help message" << std::endl;
    return true;
}
std::map<std::string,bool (*)(int argc, char* argv[],AudioDevice& speaker)> commands = {
    {"mute", mute},
    {"volume", volume},
    {"list", list},
    {"setdefault", setdefault},
    {"help", help}
};



bool parseVerb(int argc, char* argv[]) {
    AudioDeviceList audioDevices;
    int i = 1;
    AudioDevice curDevice = audioDevices.DefaultSpeaker();
    std::vector<std::wstring> args;
    for (int idx = 0; idx < argc; idx++) {
        std::wstring arg(argv[i], argv[i]+strlen(argv[i]));
        args.push_back(arg);
    }
    if (argc > 1) {
        if (args[1].find(L"D:") == 0 ) {
            int pfx_offset = 2;
            std::wstring name(argv[1]+pfx_offset, argv[1]+strlen(argv[1]));
            curDevice = audioDevices.Get(name);
            i = 2;
        } else if (isdigit(argv[1][0])) {
            try {
            curDevice = audioDevices.Get(atoi(argv[1]));
            } catch(...) {
                std::cerr << "Error: speaker index out of range. " << std::endl;
                return false;
            }
            i = 2;
        }
    }
    if (!curDevice.Active() ){
        std::cerr << "Error: speaker not found" << std::endl;
        return false;
    }
    try {
        std::wcout << L"Using speaker: " << curDevice.Name() << std::endl;
        commands.at(argv[i])(argc-i, argv+i, curDevice);
    } catch(...) {
        if (volume(argc, argv, curDevice)) {return true;}
        std::cerr << "Error: command not found" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "CoInitialize failed: 0x" << std::hex << hr << std::endl;
        return 1;
    }

    return parseVerb(argc, argv);

    CoUninitialize();
    _CrtDumpMemoryLeaks();
    return 0;
}