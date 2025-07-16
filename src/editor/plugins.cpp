#define LUMIX_NO_CUSTOM_CRT
#include "core/allocator.h"
#include "editor/studio_app.h"
#include "editor/world_editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <string>
#include <variant>
#include <vector>

using namespace Lumix;

struct EditorPlugin : StudioApp::GUIPlugin
{
	EditorPlugin(StudioApp& app)
		: m_app(app)
		, frameCount(200)
		, selected_keyframe(nullptr)
		, dragging_keyframe(nullptr)
		, splitter_ratio(0.2f)
		, splitter_active(false)
		, currentFrame(20)
		, selected_track(nullptr)
	{
		
		tracks = {{"Track 1", {{10, Vec3(1, 2, 3)}, {20, Vec3(4, 5, 6)}}, Track::ValueType::Vec3},
			{"Track2", {{10, Quat(1, 0, 0, 0)}, {20, Quat(0, 1, 0, 0)}}, Track::ValueType::Quat}};
	}

	StudioApp& m_app;

	struct Keyframe
	{
		int frame;
		std::variant < float, int,Vec2, Vec3, Quat > value;
	};

	struct Track
	{
		std::string name;
		std::vector<Keyframe> keyframes;

		enum class ValueType
		{
			Float,
			Int,
			Vec2,
			Vec3,
			Quat
		};
		ValueType type;
	};

	std::vector<Track> tracks;

	Keyframe* selected_keyframe;
	Track* selected_track;
	Keyframe* dragging_keyframe;
	float drag_offset_x = 0.0f;
	int frameCount;
	int currentFrame;
	
	float splitter_ratio;
	bool splitter_active;

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

			
			float total_height = ImGui::GetContentRegionAvail().y;
			float splitter_height = 8.0f; 

			
			float timeline_height = total_height * splitter_ratio - splitter_height * 0.5f;
			float inspector_height = total_height * (1.0f - splitter_ratio) - splitter_height * 0.5f;

			ImGui::Text("Keyframe Sequencer:");

			float frameWidth = 5.0f; 

			
			ImVec2 timeline_size = ImVec2(0, timeline_height);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));
			ImGui::BeginChild("TimelineRegion", timeline_size, true);

			
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
			ImVec2 canvas_size = ImGui::GetContentRegionAvail();

			
			ImU32 bg_col = IM_COL32(40, 40, 40, 255);
			draw_list->AddRectFilled(
				canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), bg_col);

			float trackHeight = 25.0f; 

			
			ImU32 short_line_color = IM_COL32(80, 80, 80, 100);
			ImU32 long_line_color = IM_COL32(80, 80, 80, 255);	
			ImU32 player_line_color = IM_COL32(255, 255, 255, 255);

			float short_line_height = 8.0f; 

			
			for (int f = 0; f <= frameCount; ++f)
			{
				float x = canvas_pos.x + 120 + f * frameWidth; 

				draw_list->AddLine(
					ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + short_line_height), short_line_color);
			}


			
			int frame_spacing = 10;
			for (int f = 0; f <= frameCount; f += frame_spacing)
			{
				float x = canvas_pos.x + 120 + f * frameWidth;

				
				draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y), long_line_color);

				
				char buf[16];
				sprintf_s(buf, "%d", f);
				ImVec2 text_pos = ImVec2(x + 2, canvas_pos.y + 2);
				draw_list->AddText(text_pos, IM_COL32_WHITE, buf);
			}

			
			for (size_t t = 0; t < tracks.size(); ++t)
			{
				float y = canvas_pos.y + t * trackHeight + trackHeight * 0.5f;

				
				draw_list->AddText(ImVec2(canvas_pos.x + 5, y - 8), IM_COL32_WHITE, tracks[t].name.c_str());

				
				for (Keyframe& kf : tracks[t].keyframes)
				{
					float x = canvas_pos.x + 120 + kf.frame * frameWidth;

					
					ImU32 kf_color = (selected_keyframe == &kf) ? IM_COL32(255, 255, 0, 255) : 
										 IM_COL32(255, 200, 0, 255); 

					draw_list->AddCircleFilled(ImVec2(x, y), 4.0f, kf_color);

					ImRect kf_rect(ImVec2(x - 4, y - 4), ImVec2(x + 4, y + 4));

					if (ImGui::IsMouseHoveringRect(kf_rect.Min, kf_rect.Max))
					{
						if (ImGui::IsMouseHoveringRect(kf_rect.Min, kf_rect.Max) && ImGui::IsMouseClicked(0))
						{
							selected_keyframe = &kf;
							dragging_keyframe = &kf;


							ImVec2 mouse_pos = ImGui::GetMousePos();
							drag_offset_x = mouse_pos.x - x;
						}
						if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
						{
							selected_keyframe = &kf; // optional: kijelölés is legyen
							ImGui::OpenPopup("KeyframeContextMenu");
						}
					}
				}

				
				if (dragging_keyframe && ImGui::IsMouseDragging(0))
				{
					ImVec2 mouse_pos = ImGui::GetMousePos();
					float timeline_x = mouse_pos.x - drag_offset_x;
					float timeline_origin_x = canvas_pos.x + 120;

					
					int new_frame = int((timeline_x - timeline_origin_x) / frameWidth + 0.5f);
					new_frame = Lumix::clamp(new_frame, 0, frameCount);

					dragging_keyframe->frame = new_frame;
				}

				
				if (dragging_keyframe && ImGui::IsMouseReleased(0))
				{
					dragging_keyframe = nullptr;
				}
			}

			draw_list->AddLine(ImVec2(canvas_pos.x + 120 + currentFrame * frameWidth, canvas_pos.y),
				ImVec2(canvas_pos.x + 120 + currentFrame * frameWidth, canvas_pos.y + short_line_height),
				player_line_color);

			if (ImGui::BeginPopup("KeyframeContextMenu"))
			{
				ImGui::Text("Keyframe options");
				ImGui::Separator();

				if (ImGui::MenuItem("Delete"))
				{
					// Példa: töröld a kijelölt keyframe-et
					for (Track& track : tracks)
					{
						auto& keys = track.keyframes;
						keys.erase(
							std::remove_if(
								keys.begin(), keys.end(), [&](const Keyframe& k) { return &k == selected_keyframe; }),
							keys.end());
					}
					selected_keyframe = nullptr;
				}

				if (ImGui::MenuItem("Duplicate"))
				{
					if (selected_keyframe)
					{
						for (Track& track : tracks)
						{
							for (Keyframe& kf : track.keyframes)
							{
								if (&kf == selected_keyframe)
								{
									track.keyframes.push_back(kf);
									track.keyframes.back().frame += 5;
									break;
								}
							}
						}
					}
				}

				ImGui::EndPopup();
			}


			ImGui::EndChild();
			ImGui::PopStyleVar();

			ImGui::Button("Play", ImVec2(100, 0));
			ImGui::SameLine();
			ImGui::Button("Pause", ImVec2(100, 0));
			ImGui::SameLine();
			ImGui::Button("Stop", ImVec2(100, 0));
			ImGui::SameLine();
			ImGui::Button("Start", ImVec2(100, 0));
			ImGui::SameLine();
			ImGui::Button("End", ImVec2(100, 0));
			
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

			
			ImGui::Button("##splitter", ImVec2(-1, splitter_height));

			
			if (ImGui::IsItemActive() || splitter_active)
			{
				splitter_active = true;

				
				float mouse_delta = ImGui::GetIO().MouseDelta.y;
				splitter_ratio += mouse_delta / total_height;

				
				splitter_ratio = Lumix::clamp(splitter_ratio, 0.1f, 0.9f);
			}

			
			if (ImGui::IsMouseReleased(0))
			{
				splitter_active = false;
			}

			ImGui::PopStyleColor(3);

			
			if (ImGui::IsItemHovered())
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
			}

			
			ImGui::BeginChild("Inspector", ImVec2(0, inspector_height), true);

			ImGui::Text("Inspector");

			
			if (selected_keyframe)
			{
				ImGui::InputInt("Frame", &selected_keyframe->frame);

				
				std::visit(
					[&](auto& val)
					{
						using T = std::decay_t<decltype(val)>;
						if constexpr (std::is_same_v<T, float>)
						{
							ImGui::InputFloat("Value", &val);
						}
						else if constexpr (std::is_same_v<T, int>)
						{
							ImGui::DragInt("Value", &val);
						}
						else if constexpr (std::is_same_v<T, Vec2>)
						{
							ImGui::InputFloat2("Value", &val.x);
						}
						else if constexpr (std::is_same_v<T, Vec3>)
						{
							ImGui::InputFloat3("Value", &val.x);
						}
						else if constexpr (std::is_same_v<T, Quat>)
						{
							ImGui::InputFloat4("Value", &val.x);
						}
					},
					selected_keyframe->value);
			}
			else
			{
				ImGui::Text("No keyframe selected.");
				ImGui::Text("Click on a keyframe in the timeline to edit it.");
			}

			ImGui::EndChild();
		}
		ImGui::End();
	}

	Keyframe make_keyframe(int frame, Track::ValueType type)
	{
		switch (type)
		{
			case Track::ValueType::Float: return {frame, 0.0f};
			case Track::ValueType::Int: return {frame, 0};
			case Track::ValueType::Vec2: return {frame, Vec2(0, 0)};
			case Track::ValueType::Vec3: return {frame, Vec3(0, 0, 0)};
			case Track::ValueType::Quat: return {frame, Quat(0, 0, 0, 1)};
		}
		ASSERT(false);
		return {frame, 0.0f};
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