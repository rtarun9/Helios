#include "Log.hpp"

namespace helios::core
{
	void LogMessage(std::wstring_view message, const LogMessageTypes &messageType)
	{
		if (message.data() == L"")
		{
			return;
		}

		auto messageStr = WstringToString(message);
		helios::core::ApplicationLog::AddLog(std::move(messageStr), messageType);
	}

	void ApplicationLog::AddLog(std::string_view logMessage, const core::LogMessageTypes& messageType)
	{
		// note(rtarun9) : Crashes occur because of (most probably) a compiler error in MSVC, so function not in use until the cause of error is identified.
		return;

		std::string message{ logMessage };
		textBuffer.push_back(message);
		messageTypes.push_back(messageType);
	}
}