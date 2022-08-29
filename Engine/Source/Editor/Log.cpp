#include "Log.hpp"

#include "Editor/Editor.hpp"

namespace helios::editor
{
	void LogMessage(std::wstring_view message, const LogMessageTypes &messageType)
	{
		if (message.data() == L"")
		{
			return;
		}

		auto messageStr = WstringToString(message);
		helios::editor::Editor::sApplicationLog.AddLog(std::move(messageStr), messageType);
	}
}