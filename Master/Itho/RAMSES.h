/*
 * Author: Klusjesman, supersjimmie, modified and reworked by arjenhiemstra
 */

#ifndef __ITHOCC1101_H__
#define __ITHOCC1101_H__

#include <stdio.h>
#include "CC1101.h"
#include "RAMSESMessage.h"


//pa table settings
const uint8_t ithoPaTableSend[8] = {0x6F, 0x26, 0x2E, 0x8C, 0x87, 0xCD, 0xC7, 0xC0};
const uint8_t ithoPaTableReceive[8] = {0x6F, 0x26, 0x2E, 0x7F, 0x8A, 0x84, 0xCA, 0xC4};

//message command bytes
const uint8_t ithoMessageRVHighCommandBytes[] =   {49,224,4,0,0,200};
const uint8_t ithoMessageHighCommandBytes[] =     {34,241,3,0,4,4};
const uint8_t ithoMessageFullCommandBytes[] =     {34,241,3,0,4,4};
const uint8_t ithoMessageMediumCommandBytes[] =   {34,241,3,0,3,4};
const uint8_t ithoMessageRVMediumCommandBytes[] = {34,241,3,0,3,7};
const uint8_t ithoMessageLowCommandBytes[] =      {34,241,3,0,2,4};
const uint8_t ithoMessageRVLowCommandBytes[] =    {49,224,4,0,0,1};
const uint8_t ithoMessageRVAutoCommandBytes[] =   {34,241,3,0,5,7};
const uint8_t ithoMessageStandByCommandBytes[] =  {0,0,0,0,0,0};         //unkown, tbd
const uint8_t ithoMessageTimer1CommandBytes[] =   {34,243,3,0,0,10};     //10 minutes full speed
const uint8_t ithoMessageTimer2CommandBytes[] =   {34,243,3,0,0,20};     //20 minutes full speed
const uint8_t ithoMessageTimer3CommandBytes[] =   {34,243,3,0,0,30};     //30 minutes full speed
const uint8_t ithoMessageJoinCommandBytes[] =     {31,201,12,0,34,241};
const uint8_t ithoMessageJoin2CommandBytes[] =    {31,201,12,99,34,248};  //join command of RFT AUTO Co2 remote
const uint8_t ithoMessageRVJoinCommandBytes[] =   {31,201,24,0,49,224};  //join command of RFT-RV
const uint8_t ithoMessageLeaveCommandBytes[] =    {31,201,6,0,31,201};
//itho rft-rv
//unknown, high
//148,216,43,49,224,4,0,0,200,0,3,127,244,78,11,155,154,225,11,96,138
//148,216,43,49,224,4,0,0,200,0,3,127,51,80,47,233,94,6,189,114,73

//low
//148,216,43,49,224,4,0,0,1,0,202,127,242,212,160,123,15,64,7,129,33
//148,216,43,34,241,3,0,4,4,194,127,255,189,90,107,88,72,115,49,192,105

//join
//151,149,65,31,201,24,0,49,224,151,149,65,0,18,160,151,149,65,1,16,224


class RAMSES : protected CC1101
{
  public:
    RAMSES(uint8_t counter = 0, uint8_t sendTries = 3);   //set initial counter value
    ~RAMSES() {};

    // initialization
    void init() { CC1101::init(); initReceive(); }                    //init,reset CC1101
    void initReceive();
    // uint8_t getLastCounter() { return outMessage.counter; }        //counter is increased before sending a command
    void setSendTries(uint8_t sendTries) { this->sendTries = sendTries; }
    // void setDeviceID(uint8_t byte0, uint8_t byte1, uint8_t byte2) { this->outMessage.deviceId[0] = byte0; this->outMessage.deviceId[1] = byte1; this->outMessage.deviceId[2] = byte2;}

    // receiving
    bool checkForNewPacket();                       //check RX fifo for new data
    // RAMSESMessage getLastMessage() const { return inMessage; }           //retrieve last received/parsed packet from remote
    // IthoCommand getLastCommand() const { return inMessage.command; }           //retrieve last received/parsed command from remote
    // uint8_t getLastInCounter() const { return inMessage.counter; }           //retrieve last received/parsed command from remote
    // int * getLastID() const;
    // String getLastIDstr(bool ashex=true) const;
    // CC1101Packet getLastPacket() const;
    // String LastMessageDecoded() const;

    // sending
    // void sendCommand(IthoCommand command);

    // other
    uint8_t ReadRSSI();

  private:
    RAMSES( const RAMSES &c);
    RAMSES& operator=( const RAMSES &c);

    //init CC1101 for receiving
    void initReceiveMessage();

    //init CC1101 for sending
    void initSendMessage(uint8_t len);
    void finishTransfer();

    //parse received message
    int messageParse(RAMSESMessage *msg);
    int messageInterpret(const RAMSESMessage *msg);
    // bool checkIthoCommand(RAMSESMessage *itho, const uint8_t commandBytes[]);

    // sending
    void createMessageStart(RAMSESMessage *itho, CC1101Packet *packet);
    void createMessageCommand(RAMSESMessage *itho, CC1101Packet *packet);
    void createMessageJoin(RAMSESMessage *itho, CC1101Packet *packet);
    void createMessageLeave(RAMSESMessage *itho, CC1101Packet *packet);
    // uint8_t* getMessageCommandBytes(IthoCommand command);
    uint8_t getCounter2(RAMSESMessage *itho, uint8_t len);

    uint8_t messageEncode(const RAMSESMessage *itho, CC1101Packet *packet);
    int messageDecode(const CC1101Packet *packet, RAMSESMessage *itho);

    //send
    RAMSESMessage outMessage;                       //stores state of "remote"

    //settings
    uint8_t sendTries;                            //number of times a command is send at one button press

}; //RAMSES

#endif //__ITHOCC1101_H__
