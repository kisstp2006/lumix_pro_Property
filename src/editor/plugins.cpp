#define LUMIX_NO_CUSTOM_CRT
#include "core/allocator.h"
#include "editor/studio_app.h"
#include "editor/world_editor.h"

#include "imgui/imgui.h"


using namespace Lumix;

struct EditorPlugin : StudioApp::GUIPlugin {
	EditorPlugin(StudioApp& app) : m_app(app) {}

	void onGUI() override {
		ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Pro Property")) {
			ImGui::TextUnformatted("Hello world");
			ImGui::DragFloat("Some value", &m_some_value);
		}
		ImGui::End();
	}

	const char* getName() const override { return "proproperty"; }

	StudioApp& m_app;
	float m_some_value = 0;
};


LUMIX_STUDIO_ENTRY(proproperty)
{
	WorldEditor& editor = app.getWorldEditor();

	auto* plugin = LUMIX_NEW(editor.getAllocator(), EditorPlugin)(app);
	app.addPlugin(*plugin);
	return nullptr;
}
