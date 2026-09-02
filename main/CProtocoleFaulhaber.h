#pragma once
#include "Protocole.h"
class CProtocoleFaulhaber :
    public CProtocole
{
public:
    CProtocoleFaulhaber();

    virtual void EnvoieTrameQuestion(CExternalVariable* pVar);
    virtual int AttenteTrameQuestion();

private:
    int nWaitingBytes;
    uint8_t crc(uint8_t *bfc, int n);
};

