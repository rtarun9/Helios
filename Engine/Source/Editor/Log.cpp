#include "Log.hpp"

#include "Editor/Editor.hpp"

namespace helios::editor
{
	void LogMessage(std::wstring_view message, const LogMessageTypes &messageType)
	{
		helios::editor::Editor::sApplicationLog.AddLog(WstringToString(message), messageType);
	}
}