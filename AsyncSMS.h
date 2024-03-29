#ifndef _AsyncSMS_h
#define _AsyncSMS_h

#include <Arduino.h>
#include <inttypes.h>
//#include <Timers.h>
#include "Timers.h" // FS ++++++++++++++++  better if the special Timers library https://github.com/nettigo/Timers is local
#define SMS_SEND_MSG_LEN 160
#define SMS_SEND_QUEUE_LENGTH 4
#define SMS_RETRY_COUNT 2
#define GSM_CMD_QUEUE_LEN 16

#define PHONE_NUMBER_LEN 13

#define RECEIVED_MESSAGE_MAX_LENGTH 240
#define RECEIVED_SMS_MAX_LENGTH 200

#define SMS_WAITING_FOR_CMD_RESPONSE_TIMEOUT 15000

#define REGISTRATION_ERROR_CHECKS 5

//#define ASYNC_SMS_DEBUG_MODE

class AsyncSMS
{
  private:
    enum class SMSParseStageEnum {
      WaitingForNumber,
      ParsingNumber,
      WaitingForAlpha,
      ParsingAlpha,
      WaitingForDateTime,
      ParsingDateTime,
      WaitingForData,
      WaitingForData2,
      ParsingData
    };
    enum class SMSSendingStageEnum {
      Idle,
      Starting,
      SendingText,
      Finishing
    };

    struct SmsQueueItem {
      char number[PHONE_NUMBER_LEN];
      char message[SMS_SEND_MSG_LEN];
    };

    //  HardwareSerial *_gsm;
    Stream *_gsm;  // FS +++++++++++++++++++
    //   uint32_t _baudRate;    // FS +++++++++++   no longer used.
    bool _autoStateRefresh;

    char _receivedMessage[RECEIVED_MESSAGE_MAX_LENGTH];
    uint16_t _receivedMessageIndex = 0;

    SmsQueueItem _smsSendQueue[SMS_SEND_QUEUE_LENGTH];
    uint8_t _smsSendQueueBeginIndex = 0;
    uint8_t _smsSendQueueEndIndex = 0;
    SMSSendingStageEnum _sendingStage = SMSSendingStageEnum::Idle;
    uint8_t _smsSendRetry = 0;

    Timers _receivingMessageTimer;
    Timers _waitingForResponseTimer;
    Timers _stateRefreshTimer;

    bool _waitingForResponse;
    bool _receiving;
    String _cmdQueue[GSM_CMD_QUEUE_LEN];
    uint8_t _cmdQueueStart;
    uint8_t _cmdQueueEnd;

    uint8_t _state[4];
    uint8_t _resValues[4];
    uint8_t _registrationErrorsCount;

    void reinitGSMModuleConnection();

    void enqueue(String cmd);
    void enqueueSMS(String cmd);
    void enqueueWithoutNewLine(String cmd);
    String dequeue();
    bool queueAvailable();

    bool checkFunctionResult(String toCheck);
    bool isNewSMS();
    bool isSMSSend();
    bool isOK();
    bool isError();
    String getResultParameter(String cmd);
    uint8_t parseResultValues(String res);
    void handleCommandResponse();
    void parseSMS();
    void checkRegistrationState(uint8_t registrationEnabledState, uint8_t registrationState);

    void retrySMSSend();
    void processSMSSending();
    void clearSMSBuffer();

    uint16_t findLineBreak(char *_msg, uint16_t len);

    void log(const char *msg);  // FS +++   const added
    void log(String msg);
  public:
    char myPhoneNumber[PHONE_NUMBER_LEN]="unknown nbr";  // FS +++++++++++++++++++++++++++++
    //AsyncSMS(HardwareSerial *gsm, uint32_t baudRate) : AsyncSMS(gsm, baudRate, false) { }
    //AsyncSMS(HardwareSerial *gsm, uint32_t baudRate, bool autoStateRefresh);
    AsyncSMS(Stream *gsm, uint32_t baudRate) = delete ;   // FS ++++++++++
    AsyncSMS(Stream *gsm) : AsyncSMS(gsm, false) { }  // FS ++++++++++
    AsyncSMS(Stream *gsm, bool autoStateRefresh);  // FS ++++++++++

    void init();
    void send(String number, char *message, uint8_t len);
    void send(String number, char *message) {
      send( number, message, strlen(message));
    };  // FS +++++++++++++
    void process();
    void deleteAllSMS();
    uint8_t fillState(uint8_t index, uint8_t* state);

    void (*smsReceived)(char * number, char * message);
    //  void (*logger)(char *msg);
    void (*logger)(const char *msg);      //  FS  ++++++++++++++++  sinon erreur compil dans void AsyncSMS::log(String msg) {
};

#endif
