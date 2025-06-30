from PIL import Image, ImageFile
import os

Image.MAX_IMAGE_PIXELS = None
ImageFile.LOAD_TRUNCATED_IMAGES = True


def split_image_to_tiles_lazy(input_path, output_dir, tile_size):
    os.makedirs(output_dir, exist_ok=True)

    with Image.open(input_path) as img:
        width, height = img.size

        tiles_x = (width + tile_size - 1) // tile_size
        tiles_y = (height + tile_size - 1) // tile_size

        print(
            f"Dzielenie obrazu {width}x{height} na {tiles_x}x{tiles_y} kafelków po {tile_size}x{tile_size}")

        for y in range(tiles_y):
            for x in range(tiles_x):
                left = x * tile_size
                upper = y * tile_size
                right = min(left + tile_size, width)
                lower = min(upper + tile_size, height)

                img_crop = img.crop((left, upper, right, lower))

                tile_filename = os.path.join(output_dir, f"tile_{x}_{y}.jpg")
                img_crop.save(tile_filename)

                print(f"Zapisano: {tile_filename}")


image_path = "earth.jpg"
output_dir = "tiles"
tile_size = 1024

split_image_to_tiles_lazy(image_path, output_dir, tile_size)
