#pragma once

#include "i18n.hpp"
#include "inttypes.hpp"

class GameErrorContext;

class GameErrorContext
{
  public:
    char m_Buffer[0x800];
    char *m_BufferEnd;
    i8 m_ShowMessageBox;

    GameErrorContext()
    {
        m_BufferEnd = m_Buffer;
        m_Buffer[0] = '\0';

        m_ShowMessageBox = false;
        Log(TH_ERR_LOGGER_START);
    }

    ~GameErrorContext()
    {
    }

    void ResetContext()
    {
        m_BufferEnd = m_Buffer;
        m_BufferEnd[0] = '\0';

    }

    void Flush();

    const char *Fatal(const char *fmt, ...);
    const char *Log(const char *fmt, ...);
};

extern GameErrorContext g_GameErrorContext;
