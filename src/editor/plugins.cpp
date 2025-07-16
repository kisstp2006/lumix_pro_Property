#define LUMIX_NO_CUSTOM_CRT
#include "core/allocator.h"
#include "editor/studio_app.h"
#include "editor/world_editor.h"
#include "imgui/imgui.h"
#include <vector>

using namespace Lumix;

struct EditorPlugin : StudioApp::GUIPlugin
{
	EditorPlugin(StudioApp& app)
		: m_app(app)
		, animation_frame(0) // Initialize to 0
	{
	}

	StudioApp& m_app;
	int animation_frame; // Member variable to persist between frames

	std::vector<float> times;
	std::vector<Vec3> positions;
	std::vector<Quat> rotations;
	std::vector<Vec3> scales;
	std::vector<uint32_t> jointIndices;

	void onGUI() override
	{
		WorldEditor& editor = m_app.getWorldEditor();
		const Array<EntityRef>& ents = editor.getSelectedEntities();
		World& world = *editor.getWorld();

		const char* entity_name = "No entity selected";

		// Check if there are selected entities
		if (!ents.empty())
		{
			entity_name = world.getEntityName(ents[0]);
		}

		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver); // Wider window
		if (ImGui::Begin("Pro Property"))
		{
			ImGui::Text("Selected Entity: %s", entity_name);

			// Show other info
			if (!ents.empty())
			{
				ImGui::Text("Entity ID: %d", ents[0].index);
				ImGui::Text("Selected entities: %d", ents.size());
			}

			ImGui::Separator();

			// Label for input field
			ImGui::Text("Animation Frame:");
			ImGui::InputInt("##animation_frame", &animation_frame); // & operator to pass address
		}
		ImGui::End();
	}

	const char* getName() const override { return "proproperty"; }
};

LUMIX_STUDIO_ENTRY(proproperty)
{
	WorldEditor& editor = app.getWorldEditor();
	auto* plugin = LUMIX_NEW(editor.getAllocator(), EditorPlugin)(app);
	app.addPlugin(*plugin);
	return nullptr;
}