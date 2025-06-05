import os
from PIL import Image

# Get the folder path of the script
folder_path = os.path.dirname(os.path.abspath(__file__))

# Loop through all files in the folder
for filename in os.listdir(folder_path):
    if filename.lower().endswith('.png') and not filename.endswith('_resized.png'):
        image_path = os.path.join(folder_path, filename)
        with Image.open(image_path) as img:
            # Calculate new size
            new_size = (img.width // 2, img.height // 2)
            resized_img = img.resize(new_size, Image.LANCZOS)

            # Build new file name
            base_name = os.path.splitext(filename)[0]
            new_filename = f"{base_name}_resized.png"
            new_path = os.path.join(folder_path, new_filename)

            # Save resized image
            resized_img.save(new_path)

print("Resizing complete.")
