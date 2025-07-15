#define LUMIX_NO_CUSTOM_CRT
#include "core/allocator.h"
#include "editor/studio_app.h"
#include "editor/world_editor.h"

#include "imgui/imgui.h"
#include <vector> // Include necessary header for std::vector

using namespace Lumix;

/*
// we dont need these for now, but we can implement them later if needed
enum class PropertyType
{
	FLOAT,
	VEC3,
	VEC4,
	COLOR,
	QUATERNION,
	BOOL
};
*/
struct Keyframe {
    float time; // Define the structure for a keyframe
};

struct EditorPlugin : StudioApp::GUIPlugin {
	EditorPlugin(StudioApp& app) : m_app(app) {}

	void onGUI() override {
		ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Pro Property")) {
			ImGui::TextUnformatted("Hello world");
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
			ImVec2 canvas_size = ImGui::GetContentRegionAvail();

			float total_time = 1.0f; // Assuming 1.0f represents the total duration of the timeline

			// Keyframe drawing
			for (auto& keyframe : keyframes) {
				float x = canvas_pos.x + (keyframe.time / total_time) * canvas_size.x;
				draw_list->AddCircleFilled(ImVec2(x, canvas_pos.y + 20), 5, IM_COL32(255, 0, 0, 255));
			}
		}
		ImGui::End();
	}

	const char* getName() const override { return "proproperty"; }

	StudioApp& m_app;
	float m_some_value = 0;

	// Add a member variable to store keyframes
	std::vector<Keyframe> keyframes = {
		{0.2f}, {0.5f}, {0.8f} // Example keyframes with time values
	};
};


LUMIX_STUDIO_ENTRY(proproperty)
{
	WorldEditor& editor = app.getWorldEditor();

	auto* plugin = LUMIX_NEW(editor.getAllocator(), EditorPlugin)(app);
	app.addPlugin(*plugin);
	return nullptr;
}
