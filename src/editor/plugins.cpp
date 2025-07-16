#define LUMIX_NO_CUSTOM_CRT
#include "core/allocator.h"
#include "editor/studio_app.h"
#include "editor/world_editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <string>
#include <vector>

using namespace Lumix;

struct EditorPlugin : StudioApp::GUIPlugin
{
	EditorPlugin(StudioApp& app)
		: m_app(app)
		, frameCount(100)
		, selected_keyframe(nullptr)
	{
		// Példa track adatok
		tracks = {{"Position", {{10, 1.0f}, {30, 2.0f}, {50, 3.0f}}}, {"Rotation", {{20, 0.5f}, {51, 1.0f}}}};
	}

	StudioApp& m_app;

	struct Keyframe
	{
		int frame;
		float value;
	};

	struct Track
	{
		std::string name;
		std::vector<Keyframe> keyframes;
	};

	std::vector<Track> tracks;

	Keyframe* selected_keyframe = nullptr;
	int frameCount;

	void onGUI() override
	{
		WorldEditor& editor = m_app.getWorldEditor();
		const Array<EntityRef>& ents = editor.getSelectedEntities();
		World& world = *editor.getWorld();

		const char* entity_name = "No entity selected";
		if (!ents.empty())
		{
			entity_name = world.getEntityName(ents[0]);
		}

		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Pro Property"))
		{
			ImGui::Text("Selected Entity: %s", entity_name);
			ImGui::Separator();

			ImGui::Text("Keyframe Sequencer:");

			float timeline_height = 200.0f;
			ImVec2 timeline_size = ImVec2(0, timeline_height);

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));
			ImGui::BeginChild("TimelineRegion", timeline_size, true);

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
			ImVec2 canvas_size = timeline_size;

			ImU32 bg_col = IM_COL32(40, 40, 40, 255);
			draw_list->AddRectFilled(
				canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), bg_col);

			float trackHeight = 25.0f;
			float frameWidth = 5.0f;

			for (size_t t = 0; t < tracks.size(); ++t)
			{
				float y = canvas_pos.y + t * trackHeight + trackHeight * 0.5f;

				draw_list->AddText(ImVec2(canvas_pos.x + 5, y - 8), IM_COL32_WHITE, tracks[t].name.c_str());

				for (Keyframe& kf : tracks[t].keyframes)
				{
					float x = canvas_pos.x + 120 + kf.frame * frameWidth;

					draw_list->AddCircleFilled(ImVec2(x, y), 4.0f, IM_COL32(255, 200, 0, 255));

					ImRect kf_rect(ImVec2(x - 4, y - 4), ImVec2(x + 4, y + 4));
					if (ImGui::IsMouseHoveringRect(kf_rect.Min, kf_rect.Max) && ImGui::IsMouseClicked(0))
					{
						selected_keyframe = &kf;
					}
				}
			}

			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::Separator();
			ImGui::Text("Inspector");

			if (selected_keyframe)
			{
				ImGui::InputInt("Frame", &selected_keyframe->frame);
				ImGui::InputFloat("Value", &selected_keyframe->value);
			}
			else
			{
				ImGui::Text("No keyframe selected");
			}
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
