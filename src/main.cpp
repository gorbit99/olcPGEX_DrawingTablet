#include <thread>
#define OLC_PGE_APPLICATION
#include "olcPGEX_DrawingTablet.h"
#include "olcPixelGameEngine.h"

class Example : public olc::PixelGameEngine {
public:
    Example() {
        sAppName = "Drawing Tablet Example";
    }

    bool OnUserCreate() override {
        tablet = DrawingTabletManager::Get()->GetTablet();
        draw_sprite =
                new olc::Sprite(ScreenWidth() - 100, ScreenHeight() - 200);
        return true;
    }

    std::shared_ptr<DrawingTablet> tablet;
    olc::Sprite *draw_sprite = nullptr;
    float previous_pressure = 0;
    olc::vi2d previous_position = {0, 0};

    bool OnUserUpdate(float fElapsedTime) override {
        Clear(olc::BLACK);
        auto position = tablet->GetNormalizedPosition();
        auto pressure = tablet->GetPressure();
        DrawString(0,
                   0,
                   std::to_string(position.x) + ", "
                           + std::to_string(position.y),
                   olc::WHITE);

        if (tablet->GetButton(1).bHeld) {
            SetDrawTarget(draw_sprite);

            auto color = olc::RED;
            if (tablet->GetButton(3).bHeld) {
                color = olc::BLACK;
            }

            olc::vf2d movement_vector = GetMousePos() - previous_position;
            olc::vf2d movement_vector_normalized = movement_vector.norm();
            olc::vf2d perpendicular_vector = movement_vector_normalized.perp();

            olc::vi2d start_left =
                    previous_position
                    - perpendicular_vector * 10 * previous_pressure
                    - olc::vi2d{50, 150};
            olc::vi2d start_right =
                    previous_position
                    + perpendicular_vector * 10 * previous_pressure
                    - olc::vi2d{50, 150};
            olc::vi2d end_left = GetMousePos()
                                 - perpendicular_vector * 10 * pressure
                                 - olc::vi2d{50, 150};
            olc::vi2d end_right = GetMousePos()
                                  + perpendicular_vector * 10 * pressure
                                  - olc::vi2d{50, 150};
            FillTriangle(start_left, start_right, end_left, color);
            FillTriangle(start_right, end_right, end_left, color);

            SetDrawTarget(nullptr);
        }

        previous_position = GetMousePos();
        previous_pressure = pressure;

        DrawLine(50, 149, 50 + draw_sprite->width, 149, olc::WHITE, 0xf0f0f0f0);
        DrawLine(50,
                 150 + draw_sprite->height,
                 50 + draw_sprite->width,
                 150 + draw_sprite->height,
                 olc::WHITE,
                 0xf0f0f0f0);
        DrawLine(49,
                 150,
                 49,
                 150 + draw_sprite->height,
                 olc::WHITE,
                 0xf0f0f0f0);
        DrawLine(50 + draw_sprite->width,
                 150,
                 50 + draw_sprite->width,
                 150 + draw_sprite->height,
                 olc::WHITE,
                 0xf0f0f0f0);
        DrawSprite(50, 150, draw_sprite);

        for (size_t i = 0; i < tablet->GetPossibleButtonCount(); i++) {
            if (tablet->GetButton(i).bHeld) {
                FillRect(olc::vi2d(i * 20, 20), olc::vi2d(20, 20), olc::GREEN);
            }
        }

        return true;
    }
};

int main() {
    Example demo;

    if (demo.Construct(640, 480, 2, 2, false, false)) {
        demo.Start();
    }

    return 0;
}
