#include <queue>
#include <tuple>

#include <Siv3D.hpp> // OpenSiv3D v0.6.3

namespace
{
	class State
	{
	public:
		HSV brushColor{};
		Point prevMousePos{};
		double brushSize{};
		bool requestClearCanvas{};
		size_t brushIndex{};
	};

	void ProcessGui(State* pState)
	{
		SimpleGUI::Slider(U"{:.2f}"_fmt(pState->brushSize), pState->brushSize, 0.0, 100.0, Vec2{ 450, 0 }, 60, 150.0);
		if (SimpleGUI::Button(U"Clear", Vec2{ 450, 50 }))
		{
			pState->requestClearCanvas = true;
		}
		SimpleGUI::ColorPicker(pState->brushColor, Vec2{ 450, 100 });
		SimpleGUI::RadioButtons(pState->brushIndex, { U"Normal", U"Fill" }, Vec2{ 450, 250 });
	}

	class TraversalItem
	{
	public:
		int x{};
		int y{};
		Color prevColor{};
	};

	void TryAdd(std::vector<TraversalItem>* queue, int* last, int x, int y, const Point& center, double r, const Color& color, const Image& filled)
	{
		auto p = Point{ x, y };
		if (0 <= x && x < filled.width() &&
			0 <= y && y < filled.height() &&
			(p - center).length() <= r && filled[p].r == 0 && filled[p].g == 0)
		{
			(*queue)[++(*last)] = TraversalItem{ x, y, color };
		}
	}
}

void Main()
{
	State state{};
	state.brushColor = HSV{ 0,0,0 };
	state.brushSize = 1.0;
	Image filled{ 400, 400, Color{0,0,0,0} };
	Image renderTextureImage{ 400, 400, Color{0,0,0,0} };
	const RenderTexture renderTexture{ 400, 400, Palette::White };
	DynamicTexture filledTexture{ 400,400 };

	std::vector<TraversalItem> queue{};
	queue.resize(400 * 400);

	while (System::Update())
	{
		auto pos = Cursor::Pos();
		if (state.brushIndex == 0)
		{
			// Normal ブラシ
			// このスコープ内でのみレンダーターゲットを renderTexture にする
			const ScopedRenderTarget2D renderTarget{ renderTexture };
			if (MouseL.pressed())
			{
				int len = (pos - state.prevMousePos).length();
				for (int i = 0; i < len; i++)
				{
					auto p = (static_cast<double>(len - i) * state.prevMousePos + i * pos) / len;
					Circle{ p.x, p.y, state.brushSize / 2 }.draw(state.brushColor);
				}
			}
			state.prevMousePos = pos;
		}
		else if (state.brushIndex == 1)
		{
			// Fill ブラシ（fill される範囲を描画するだけ）
			renderTexture.readAsImage(renderTextureImage);
			filled.fill(Color{ 0,0,0,0 });
			int x = static_cast<int>(pos.x);
			int y = static_cast<int>(pos.y);
			Point center{ x, y };
			int last = -1;
			queue[++last] = TraversalItem{ x, y, renderTextureImage.getPixel(Point{x, y}, ImageAddressMode::Clamp) };
			while (last >= 0)
			{
				auto seek = queue[last--];
				auto color = renderTextureImage.getPixel(Point{ seek.x, seek.y }, ImageAddressMode::Clamp);
				const double Threshold = 10;
				if ((Vec3{ color.r, color.g, color.b } - Vec3{ seek.prevColor.r, seek.prevColor.g, seek.prevColor.b }).length() < Threshold)
				{
					filled[Point{ seek.x, seek.y }] = Color{ 255, 0, 0, 32 };
					TryAdd(&queue, &last, seek.x + 1, seek.y, center, state.brushSize / 2, color, filled);
					TryAdd(&queue, &last, seek.x - 1, seek.y, center, state.brushSize / 2, color, filled);
					TryAdd(&queue, &last, seek.x, seek.y + 1, center, state.brushSize / 2, color, filled);
					TryAdd(&queue, &last, seek.x, seek.y - 1, center, state.brushSize / 2, color, filled);
				}
				else
				{
					filled[Point{ seek.x, seek.y }] = Color{ 0, 255, 0, 32 };
				}
			}
			filledTexture.fill(filled);
		}

		if (state.requestClearCanvas)
		{
			renderTexture.clear(Palette::White);
			state.requestClearCanvas = false;
		}

		renderTexture.draw(0, 0);
		filledTexture.draw(0, 0);

		// ブラシプレビュー表示
		Circle{ pos.x, pos.y, state.brushSize / 2 }.drawFrame(1.0, Palette::Red);

		ProcessGui(&state);
	}
}
