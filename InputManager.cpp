/// \file
/// \author Xavier Michelon
///
/// \brief Declaration of input manager
///  
/// Copyright (c) Xavier Michelon. All rights reserved.  
/// Licensed under the MIT License. See LICENSE file in the project root for full license information.  


#include "stdafx.h"
#include "InputManager.h"
#include "MainWindow.h"
#include "Beeftext/BeeftextGlobals.h"
#include <XMiLib/Exception.h>
#include <XMiLib/SystemUtils.h>


using namespace xmilib;


namespace {
   InputManager::KeyStroke const kNullKeyStroke = {0, 0, {0}}; ///< A null keystroke
   qint32 const kTextBufferSize = 10; ///< The size of the buffer that will receive the text resulting from the processing of the key stroke
}


//**********************************************************************************************************************
/// This static member function is registered to be called whenever a key event occurs.
/// 
/// \param[in] nCode A code the hook procedure uses to determine how to process the message
/// \param[in] wParam The identifier of the keyboard message
/// \param[in] lParam A pointer to a KBDLLHOOKSTRUCT structure.
//**********************************************************************************************************************
LRESULT CALLBACK InputManager::keyboardProcedure(int nCode, WPARAM wParam, LPARAM lParam)
{
   if ((WM_KEYDOWN == wParam) || (WM_SYSKEYDOWN == wParam))
   {
      KeyStroke keyStroke = kNullKeyStroke;
      KBDLLHOOKSTRUCT *keyEvent = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
      keyStroke.virtualKey = keyEvent->vkCode;
      keyStroke.scanCode = keyEvent->scanCode;
      // GetKeyboardState() do not properly report state for modifier keys if the key event in a window other that one
      // from the current process, so we need to manually fetch the valid states manually using GetKeyState()
      // We do not actually need the state of the other key, so we do not event bother calling GetKeyboardState()
      QList<quint32> keyList = { VK_SHIFT, VK_LSHIFT, VK_RSHIFT, VK_CONTROL, VK_LCONTROL,
         VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU, VK_RWIN, VK_LWIN, VK_CAPITAL };
      for (qint32 key : keyList)
         keyStroke.keyboardState[key] = GetKeyState(key);
      InputManager::instance().onKeyboardEvent(keyStroke);
   }
   return CallNextHookEx(nullptr, nCode, wParam, lParam);
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
LRESULT CALLBACK InputManager::mouseProcedure(int nCode, WPARAM wParam, LPARAM lParam)
{
   if ((WM_LBUTTONDOWN == wParam) || (WM_RBUTTONDOWN == wParam) || (WM_MOUSEWHEEL == wParam) 
      || (WM_MOUSEWHEEL == wParam) || (WM_MBUTTONDOWN == wParam))
   {
      InputManager::instance().onMouseClickEvent(nCode, wParam, lParam);
   }
   return CallNextHookEx(nullptr, nCode, wParam, lParam);
}


//**********************************************************************************************************************
/// \return The only allowed instance of the class
//**********************************************************************************************************************
InputManager& InputManager::instance()
{
   static InputManager instance; 
   return instance;
}


//**********************************************************************************************************************
///
//**********************************************************************************************************************
InputManager::InputManager() 
   : QObject(nullptr)
   , deadKey_(kNullKeyStroke)
{
   connect(this, &InputManager::info, &globals::debugLog(), &DebugLog::addInfo);
   HMODULE moduleHandle = GetModuleHandle(nullptr);
   keyboardHook_ = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProcedure, moduleHandle, 0);
   if (!keyboardHook_)
      throw xmilib::Exception("Could not register a keyboard hook.");
   mouseHook_ = SetWindowsHookEx(WH_MOUSE_LL, mouseProcedure, moduleHandle, 0);
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
InputManager::~InputManager()
{
   if (keyboardHook_) 
      UnhookWindowsHookEx(keyboardHook_);
   if (mouseHook_)
      UnhookWindowsHookEx(mouseHook_);
}



//**********************************************************************************************************************
/// \param[in] keyStroke The key stroke
//**********************************************************************************************************************
void InputManager::onKeyboardEvent(KeyStroke const& keyStroke)
{
   QString text = this->processKey(keyStroke);
   emit info(QString("A Key was pressed. vkCode = 0x%1 - scanCode = %2 - size = %3 - text = %4")
      .arg(keyStroke.virtualKey, 2, 16, QChar('0')).arg(keyStroke.scanCode).arg(text.size()).arg(text));
}


//**********************************************************************************************************************
/// This function is more complex than it should be because dead keys that are present on layouts such as 
/// the US-International keyboard require a special treatment.
///
/// \param[in] keyStroke The key stroke
/// \return The text resulting of the keystroke
//**********************************************************************************************************************
QString InputManager::processKey(KeyStroke const& keyStroke)
{
   // now the tricky part: ToUnicode() "consumes" the dead key that may be stored in the kernel-mode keyboard buffer
   // so we need to manually restore the dead key by calling ToUnicode() again
   WCHAR textBuffer[kTextBufferSize];
   qint32 size = ToUnicode(keyStroke.virtualKey, keyStroke.scanCode, keyStroke.keyboardState, textBuffer, 
      kTextBufferSize, 0);

   if (-1 == size)
   {
      // the key is a dead key. We have consumed it so we need to:
      // 1 - Restore it by repeating the call to ToUnicode()
      // 2 - Save the key because we will need to apply it again before the next 'normal' keystroke
      ToUnicode(keyStroke.virtualKey, keyStroke.scanCode, keyStroke.keyboardState, textBuffer, kTextBufferSize, 0);
      deadKey_ = keyStroke;
      return QString();
   }

   if (size > 0)
   {
      // The key is a normal key that will result in text output.
      // if the previous key was a dead key, we have already consumed the dead key so we must restore it 
      QString result = QString::fromWCharArray(textBuffer, kTextBufferSize);
      if (0 != deadKey_.virtualKey)
      {
         ToUnicode(deadKey_.virtualKey, deadKey_.scanCode, deadKey_.keyboardState, textBuffer, kTextBufferSize, 0);
         deadKey_.virtualKey = 0;
      }
      return result;
   }

   // final case: size is 0, the key is a modifier, we do nothing
   // values of size < -1 also lead here but should not happen according to the documentation for ToUnicode()
   return QString();
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void InputManager::onMouseClickEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
   emit info("Mouse clicked.");
}


