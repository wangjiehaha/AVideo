#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring

JNICALL
Java_etrans_com_avideo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

class HolidayRequest
{
public:
    HolidayRequest(int hours) : m_iHour(hours){}
    int GetHour()
    {
        return m_iHour;
    }
private:
    int m_iHour;
};

class Manager
{
public:
    virtual bool HandlerRequest(HolidayRequest *holidayRequest) = 0;
};

class DM : public Manager
{
public:
    DM(Manager *manager):m_pHandler(manager) {

    }
    bool HandlerRequest(HolidayRequest *holidayRequest)
    {
        return true;
    }

    bool IsIn()
    {
        return true;
    }
private:
    Manager *m_pHandler;
};

class PM : public Manager
{
public:
    PM(Manager *manager) : m_pHandler(manager){}

    bool HandlerRequest(HolidayRequest *holidayRequest)
    {
        if (holidayRequest->GetHour() < 2 || holidayRequest == NULL)
        {
            return true;
        }
        return m_pHandler->HandlerRequest(holidayRequest);
    }
private:
    Manager *m_pHandler;
};

class PS : public Manager
{
public:
    PS(Manager *manager) : m_pHandler(manager){}
    bool HandlerRequest(HolidayRequest *holidayRequest)
    {
        if (holidayRequest->GetHour() < 10)
        {
            return true;
        }
        return m_pHandler->HandlerRequest(holidayRequest);
    }
private:
    Manager *m_pHandler;
};

int main()
{
    Manager *pDM = new DM(NULL);
    Manager *pPs = new PS(pDM);
    Manager *pPM = new PM(pPs);
    HolidayRequest *holidayRequest = new HolidayRequest(10);
    pPM->HandlerRequest(holidayRequest);
}


class Receiver
{
public:
    void Action()
    {

    }
};

class Command
{
public:
    virtual void Execute() = 0;
};

class ConcreteCommand : public Command
{
public:
    ConcreteCommand(Receiver *pReceiver) : m_pReceiver(pReceiver){}
    void Execute()
    {
        m_pReceiver->Action();
    }
private:
    Receiver *m_pReceiver;
};

class Invoker
{
public:
    Invoker(Command *pCommand) : m_pCommand(pCommand){}
    void Invoke()
    {
        m_pCommand->Execute();
    }

private:
    Command *m_pCommand;
};

