"""격납고 목록의 누락 아이콘용 전술 placeholder를 생성한다."""

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = (
    ROOT
    / "assets"
    / "ui_hangar"
    / "output"
    / "textures"
    / "T_UI_Hangar_ItemPlaceholder.png"
)


def main() -> None:
    size = 128
    cyan = (54, 220, 238, 230)
    cyan_dim = (22, 124, 145, 170)
    fill = (3, 18, 29, 180)

    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    draw.rounded_rectangle((8, 8, 119, 119), radius=5, fill=fill, outline=cyan_dim, width=2)
    draw.ellipse((28, 28, 99, 99), outline=cyan_dim, width=2)
    draw.line((64, 17, 64, 43), fill=cyan, width=3)
    draw.line((64, 85, 64, 111), fill=cyan, width=3)
    draw.line((17, 64, 43, 64), fill=cyan, width=3)
    draw.line((85, 64, 111, 64), fill=cyan, width=3)

    # 기체와 무장 모두에 사용할 수 있는 단순한 레이더 표식을 그린다.
    draw.polygon(((64, 43), (75, 77), (64, 71), (53, 77)), outline=cyan, fill=(8, 61, 75, 220))
    draw.ellipse((59, 59, 69, 69), fill=cyan)

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    image.save(OUTPUT)
    print(OUTPUT)


if __name__ == "__main__":
    main()
