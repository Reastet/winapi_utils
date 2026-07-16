#include <Windows.h>
#include <string>
#include <iostream>
#include <Wscapi.h>
#pragma comment(lib, "Wscapi.lib")
using namespace std;



string str_firewall("FIREWALL:          ");
string str_autoupdate("AUTOUPDATE:        ");
string str_antivirus("ANTIVIRUS:         ");
string str_spyware("ANTISPYWARE:       ");
string str_int("INTERNET SETTINGS: ");
string str_uac("UAC:               ");
string str_serv("SERVICE:           ");
string str_all("ALL:               ");
string out_strs[8] = {str_firewall, str_autoupdate, str_antivirus,
str_spyware, str_int, str_uac, str_serv, str_all };

struct wsc_last_status
{
    WSC_SECURITY_PROVIDER_HEALTH firewall;
    WSC_SECURITY_PROVIDER_HEALTH autoupdate;
    WSC_SECURITY_PROVIDER_HEALTH antivirus;
    WSC_SECURITY_PROVIDER_HEALTH antispyware;
    WSC_SECURITY_PROVIDER_HEALTH internet;
    WSC_SECURITY_PROVIDER_HEALTH uac;
    WSC_SECURITY_PROVIDER_HEALTH service;
    WSC_SECURITY_PROVIDER_HEALTH all;
};

wsc_last_status cur_sost;

DWORD providers[8] = {
    WSC_SECURITY_PROVIDER_FIREWALL,
    WSC_SECURITY_PROVIDER_AUTOUPDATE_SETTINGS,
    WSC_SECURITY_PROVIDER_ANTIVIRUS,
    WSC_SECURITY_PROVIDER_ANTISPYWARE,
    WSC_SECURITY_PROVIDER_INTERNET_SETTINGS,
    WSC_SECURITY_PROVIDER_USER_ACCOUNT_CONTROL,
    WSC_SECURITY_PROVIDER_SERVICE,
    WSC_SECURITY_PROVIDER_ALL
};

string status_type(WSC_SECURITY_PROVIDER_HEALTH status)
{
    switch (status)
    {
    case WSC_SECURITY_PROVIDER_HEALTH_GOOD: return "GOOD";
    case WSC_SECURITY_PROVIDER_HEALTH_NOTMONITORED: return "NOTMONITORED";
    case WSC_SECURITY_PROVIDER_HEALTH_POOR: return "POOR";
    case WSC_SECURITY_PROVIDER_HEALTH_SNOOZE: return "SNOOZE";
    default: return "Status Error";
    }
}


void print_status(string provider, WSC_SECURITY_PROVIDER_HEALTH health)
{
    cout << provider << status_type(health) << endl;
}
void printchanges(string provider, WSC_SECURITY_PROVIDER_HEALTH last, WSC_SECURITY_PROVIDER_HEALTH cur)
{
    cout << provider << "changed. Was: " << status_type(last) << " now: " << status_type(cur)<<endl;
}

WSC_SECURITY_PROVIDER_HEALTH getHealth(int i)
{
    WSC_SECURITY_PROVIDER_HEALTH health;
    WscGetSecurityProviderHealth(providers[i], &health);
    return health;
 

}

void set_up_status(wsc_last_status &sost)
{
    sost.firewall = getHealth(0);
    sost.autoupdate = getHealth(1);
    sost.antivirus = getHealth(2);
    sost.antispyware = getHealth(3);
    sost.internet = getHealth(4);
    sost.uac = getHealth(5);
    sost.service = getHealth(6);
    sost.all = getHealth(7);
}

void print_status_struct(wsc_last_status &sost)
{
    print_status(out_strs[0], sost.firewall);
    print_status(out_strs[1], sost.autoupdate);
    print_status(out_strs[2], sost.antivirus);
    print_status(out_strs[3], sost.antispyware);
    print_status(out_strs[4], sost.internet);
    print_status(out_strs[5], sost.uac);
    print_status(out_strs[6], sost.service);
    print_status(out_strs[7], sost.all);
    cout << endl;
}

void smth_changed()
{
    wsc_last_status new_val;
    set_up_status(new_val);
    cout << "All changes:" << endl;
    
    if (cur_sost.firewall != new_val.firewall)
    {
        printchanges(out_strs[0], cur_sost.firewall, new_val.firewall);
    }
    if (cur_sost.autoupdate != new_val.autoupdate)
    {
        printchanges(out_strs[1], cur_sost.autoupdate, new_val.autoupdate);
    }
    if (cur_sost.antivirus != new_val.antivirus)
    {
        printchanges(out_strs[2], cur_sost.antivirus, new_val.antivirus);
    }
    if (cur_sost.antispyware != new_val.antispyware)
    {
        printchanges(out_strs[3], cur_sost.antispyware, new_val.antispyware);
    }
    if (cur_sost.internet != new_val.internet)
    {
        printchanges(out_strs[4], cur_sost.internet, new_val.internet);
    }
    if (cur_sost.uac != new_val.uac)
    {
        printchanges(out_strs[5], cur_sost.uac, new_val.uac);
    }
    if (cur_sost.service != new_val.service)
    {
        printchanges(out_strs[6], cur_sost.service, new_val.service);
    }
    if (cur_sost.all != new_val.all)
    {
        printchanges(out_strs[7], cur_sost.all, new_val.all);
    }
    cur_sost = new_val;
    return;
}

void main()
{
    set_up_status(cur_sost);
    print_status_struct(cur_sost);
    HRESULT result = S_OK;
    HANDLE callbackRegistration = NULL;
    result = WscRegisterForChanges(NULL, &callbackRegistration, (LPTHREAD_START_ROUTINE)smth_changed, NULL);
    int status = 1;
    while (result == S_OK && status) {
        cin >> status;
    }
    WscUnRegisterChanges(&callbackRegistration);
    return;
}