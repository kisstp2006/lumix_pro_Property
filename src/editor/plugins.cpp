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
		, currentFrame(50)
		, selected_track(nullptr)
		, playing(false)
		, play_speed(24)
		, time_accumulator(0.0f)
		, is_scrubbing(false)
		, timeline_offset(false)
		, dragging_timeline(false)
		, hovering_keyframe(false)
		
	{
		
		tracks = {
			{"Object 1_Rotation", {{10, Vec3(1, 2, 3)}, {20, Vec3(4, 5, 6)}}, Track::ValueType::Vec3},
			{"Object 1_Transform", {{10, Quat(1, 0, 0, 0)}, {20, Quat(0, 1, 0, 0)}}, Track::ValueType::Quat},
			{"Object 3_Intensity", {{10, int(1)}, {24, int(14)}}, Track::ValueType::Int}
		};
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
	bool playing;
	int play_speed;
	float time_accumulator;
	bool is_scrubbing;
	float timeline_offset; // Horizontal panning offset in pixels
	float zoom = 1.0f;    
	bool dragging_timeline;
	bool hovering_keyframe;


	float splitter_ratio;
	bool splitter_active;



	// Fixed onGUI() method section with zoom and pan functionality

	void onGUI() override
	{
		WorldEditor& editor = m_app.getWorldEditor();
		const Array<EntityRef>& ents = editor.getSelectedEntities();
		World& world = *editor.getWorld();

		if (playing)
		{
			time_accumulator += ImGui::GetIO().DeltaTime;
			while (time_accumulator > 1.0f / play_speed)
			{
				currentFrame++;
				time_accumulator -= 1.0f / play_speed;
			}
			if (currentFrame > frameCount)
			{
				currentFrame = frameCount;
				playing = false;
			}
		}

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
			float splitter_height = 12.0f;
			float timeline_height = total_height * splitter_ratio - splitter_height * 0.5f;
			float inspector_height = total_height * (1.0f - splitter_ratio) - splitter_height * 0.5f;

			ImGui::Text("Keyframe Sequencer:");

			float base_frame_width = 5.0f;
			float frame_width = base_frame_width * zoom; // Apply zoom

			ImVec2 timeline_size = ImVec2(0, timeline_height);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));
			ImGui::BeginChild("TimelineRegion", timeline_size, true);

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
			ImVec2 canvas_size = ImGui::GetContentRegionAvail();

			float track_labels_width = 120.0f;
			float timeline_start_x = canvas_pos.x + track_labels_width; // Define timeline start position

			// ZOOM HANDLING - Ctrl + mouse wheel
			if (ImGui::IsWindowHovered() && ImGui::GetIO().KeyCtrl)
			{
				float wheel = ImGui::GetIO().MouseWheel;
				if (wheel != 0.0f)
				{
					ImVec2 mouse_pos = ImGui::GetMousePos();
					float mouse_timeline_x = mouse_pos.x - timeline_start_x;

					// Calculate which frame is under the mouse cursor before zoom
					float old_frame_width = base_frame_width * zoom;
					float frame_under_mouse = (mouse_timeline_x - timeline_offset) / old_frame_width;

					// Apply zoom
					float old_zoom = zoom;
					zoom += wheel * 0.1f;
					zoom = Lumix::clamp(zoom, 0.1f, 10.0f);

					// Calculate new frame width
					float new_frame_width = base_frame_width * zoom;

					// Adjust timeline_offset so the same frame stays under the mouse
					timeline_offset = mouse_timeline_x - frame_under_mouse * new_frame_width;
				}
			}

			// PAN HANDLING - Middle mouse button
			if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				if (!dragging_timeline) dragging_timeline = true;
				timeline_offset += ImGui::GetIO().MouseDelta.x;
			}
			else
			{
				dragging_timeline = false;
			}

			// Timeline offset constraints
			float max_timeline_width = frameCount * frame_width;
			float visible_timeline_width = canvas_size.x - track_labels_width;
			timeline_offset = Lumix::clamp(
				timeline_offset, -max_timeline_width + visible_timeline_width * 0.5f, visible_timeline_width * 0.5f);

			// Background
			ImU32 bg_col = IM_COL32(40, 40, 40, 255);
			draw_list->AddRectFilled(
				canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), bg_col);

			float trackHeight = 25.0f;
			ImU32 short_line_color = IM_COL32(80, 80, 80, 100);
			ImU32 long_line_color = IM_COL32(80, 80, 80, 255);
			ImU32 player_line_color = IM_COL32(255, 255, 255, 255);
			float short_line_height = 8.0f;

			// Calculate visible frame range for optimization
			timeline_start_x = canvas_pos.x + track_labels_width;
			float visible_start_frame = (-timeline_offset) / frame_width;
			float visible_end_frame = (-timeline_offset + canvas_size.x - track_labels_width) / frame_width;

			int start_frame = Lumix::maximum(0, int(visible_start_frame) - 1);
			int end_frame = Lumix::minimum(frameCount, int(visible_end_frame) + 1);

			// Short lines at every frame (only draw visible ones)
			if (zoom > 0.5f) // Only show short lines when zoomed in enough
			{
				for (int f = start_frame; f <= end_frame; ++f)
				{
					float x = timeline_start_x + f * frame_width + timeline_offset;

					if (x >= timeline_start_x && x <= canvas_pos.x + canvas_size.x)
					{
						draw_list->AddLine(
							ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + short_line_height), short_line_color);
					}
				}
			}

			// Long lines and frame numbers with adaptive spacing
			int base_spacing = 10;
			if (zoom < 0.5f)
				base_spacing = 50;
			else if (zoom < 1.0f)
				base_spacing = 20;
			else if (zoom > 3.0f)
				base_spacing = 5;

			int frame_spacing = base_spacing;

			// Ensure spacing is reasonable
			while (frame_spacing * frame_width < 30.0f && frame_spacing < frameCount / 4)
			{
				frame_spacing *= 2;
			}

			// Draw major frame lines and numbers
			for (int f = 0; f <= frameCount; f += frame_spacing)
			{
				float x = timeline_start_x + f * frame_width + timeline_offset;

				if (x >= timeline_start_x - 50 && x <= canvas_pos.x + canvas_size.x + 50)
				{
					draw_list->AddLine(
						ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y), long_line_color);

					// Only draw frame number if there's enough space
					if (frame_spacing * frame_width > 25.0f)
					{
						char buf[16];
						sprintf_s(buf, "%d", f);
						ImVec2 text_pos = ImVec2(x + 2, canvas_pos.y + 2);
						draw_list->AddText(text_pos, IM_COL32_WHITE, buf);
					}
				}
			}

			// Current frame line (only in timeline area)
			float current_frame_x = timeline_start_x + currentFrame * frame_width + timeline_offset;
			if (current_frame_x >= timeline_start_x - 10 && current_frame_x <= canvas_pos.x + canvas_size.x + 10)
			{
				// Only draw the line in the timeline area (from timeline_start_x down)
				draw_list->AddLine(ImVec2(current_frame_x, canvas_pos.y),
					ImVec2(current_frame_x, canvas_pos.y + canvas_size.y),
					player_line_color,
					3);

				// Current frame line top (thicker) - also only in timeline area
				draw_list->AddLine(ImVec2(current_frame_x, canvas_pos.y),
					ImVec2(current_frame_x, canvas_pos.y + short_line_height),
					player_line_color,
					6);
			}

			// Scrubbing handling
			ImVec2 current_frame_pos_min(current_frame_x - 4, canvas_pos.y);
			ImVec2 current_frame_pos_max(current_frame_x + 4, canvas_pos.y + canvas_size.y);

			if (ImGui::IsMouseHoveringRect(current_frame_pos_min, current_frame_pos_max) && ImGui::IsMouseClicked(0))
			{
				is_scrubbing = true;
			}

			if (is_scrubbing)
			{
				playing = false;
				ImVec2 mouse_pos = ImGui::GetMousePos();
				float relative_mouse_x = mouse_pos.x - timeline_start_x - timeline_offset;
				int new_frame = int(relative_mouse_x / frame_width + 0.5f);
				currentFrame = Lumix::clamp(new_frame, 0, frameCount);

				if (!ImGui::IsMouseDown(0))
				{
					is_scrubbing = false;
				}
			}

			// Double click on timeline area
			ImVec2 timeline_area_min(timeline_start_x, canvas_pos.y);
			ImVec2 timeline_area_max(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

			if (ImGui::IsMouseHoveringRect(timeline_area_min, timeline_area_max) && ImGui::IsMouseDoubleClicked(0) &&
				!is_scrubbing)
			{
				ImVec2 mouse_pos = ImGui::GetMousePos();
				float relative_mouse_x = mouse_pos.x - timeline_start_x - timeline_offset;
				int clicked_frame = int(relative_mouse_x / frame_width + 0.5f);
				currentFrame = Lumix::clamp(clicked_frame, 0, frameCount);
			}

			// Draw tracks and keyframes
			for (size_t t = 0; t < tracks.size(); ++t)
			{
				float y = canvas_pos.y + t * trackHeight + trackHeight * 0.5f;

				// Keyframes (draw first, so they appear behind track names)
				for (Keyframe& kf : tracks[t].keyframes)
				{
					float x = timeline_start_x + kf.frame * frame_width + timeline_offset;

					// Only draw if visible
					if (x >= timeline_start_x - 10 && x <= canvas_pos.x + canvas_size.x + 10)
					{
						ImU32 kf_color =
							(selected_keyframe == &kf) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 200, 0, 255);

						draw_list->AddCircleFilled(ImVec2(x, y), 4.0f, kf_color);

						ImRect kf_rect(ImVec2(x - 4, y - 4), ImVec2(x + 4, y + 4));

						if (ImGui::IsMouseHoveringRect(kf_rect.Min, kf_rect.Max))
						{
							hovering_keyframe = true;
							if (ImGui::IsMouseClicked(0))
							{
								selected_keyframe = &kf;
								dragging_keyframe = &kf;
								selected_track = &tracks[t];

								ImVec2 mouse_pos = ImGui::GetMousePos();
								drag_offset_x = mouse_pos.x - x;
							}
							if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
							{
								selected_keyframe = &kf;
								selected_track = &tracks[t];
								ImGui::OpenPopup("KeyframeContextMenu");
							}
						}
					}
				}

				// Keyframe dragging
				if (dragging_keyframe && ImGui::IsMouseDragging(0))
				{
					ImVec2 mouse_pos = ImGui::GetMousePos();
					float timeline_x = mouse_pos.x - drag_offset_x;
					float timeline_origin_x = timeline_start_x + timeline_offset;

					int new_frame = int((timeline_x - timeline_origin_x) / frame_width + 0.5f);
					new_frame = Lumix::clamp(new_frame, 0, frameCount);

					dragging_keyframe->frame = new_frame;
				}

				if (dragging_keyframe && ImGui::IsMouseReleased(0))
				{
					dragging_keyframe = nullptr;
				}
			}

			// Draw track names LAST (so they appear on top of everything)
			for (size_t t = 0; t < tracks.size(); ++t)
			{
				float y = canvas_pos.y + t * trackHeight + trackHeight * 0.5f;

				// Track name background (full height of track)
				ImVec2 text_pos = ImVec2(canvas_pos.x + 5, y - 8);

				// Background rectangle behind track name (covers full track height)
				ImU32 track_bg_color = IM_COL32(50, 50, 50, 255); // Darker background for better contrast
				if (selected_track == &tracks[t])
				{
					track_bg_color = IM_COL32(70, 70, 110, 255); // Bluish background if selected
				}

				// Full track height background
				ImVec2 bg_min = ImVec2(canvas_pos.x, canvas_pos.y + t * trackHeight);
				ImVec2 bg_max = ImVec2(timeline_start_x, canvas_pos.y + (t + 1) * trackHeight);
				draw_list->AddRectFilled(bg_min, bg_max, track_bg_color);

				// Border around track name area
				ImU32 border_color = IM_COL32(120, 120, 120, 255);
				draw_list->AddRect(bg_min, bg_max, border_color);

				// Track name text
				draw_list->AddText(text_pos, IM_COL32_WHITE, tracks[t].name.c_str());
			}

			// Keyframe context menu
			if (ImGui::BeginPopup("KeyframeContextMenu"))
			{
				ImGui::Text("Keyframe options");
				ImGui::Separator();

				if (ImGui::MenuItem("Delete"))
				{
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
					if (selected_keyframe && selected_track)
					{
						Keyframe new_kf = *selected_keyframe;
						new_kf.frame += 5;
						selected_track->keyframes.push_back(new_kf);
					}
				}

				ImGui::EndPopup();
			}

			ImGui::EndChild();
			ImGui::PopStyleVar();

			// Control buttons
			ImGui::Separator();
			ImGui::SameLine();
			if (ImGui::Button("Back.", ImVec2(100, 0)))
			{
				currentFrame = Lumix::clamp(0, currentFrame - 1, frameCount);
			}
			ImGui::SameLine();
			if (ImGui::Button("Play", ImVec2(100, 0)))
			{
				playing = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Fov.", ImVec2(100, 0)))
			{
				currentFrame = Lumix::clamp(0, currentFrame + 1, frameCount);
			}
			ImGui::SameLine();
			if (ImGui::Button("Pause", ImVec2(100, 0)))
			{
				playing = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop", ImVec2(100, 0)))
			{
				playing = false;
				currentFrame = 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("Start", ImVec2(100, 0)))
			{
				currentFrame = 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("End", ImVec2(100, 0)))
			{
				currentFrame = frameCount;
			}

			// Splitter
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

			// Inspector
			ImGui::BeginChild("Inspector", ImVec2(0, inspector_height), true);

			ImGui::Text("Inspector");
			ImGui::Text("Zoom: %.2f", zoom);
			ImGui::Text("Timeline Offset: %.2f", timeline_offset);

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

	void SetProperties(Quat rot) {

	}
	Quat GetRotation() 
	{ 
		return Quat(0, 0, 0, 0); 
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