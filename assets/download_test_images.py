import urllib.request
import os
import time

def download_images(count=100, output_dir="test_images"):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    print(f"Downloading {count} images to '{output_dir}'...")
    
    for i in range(count):
        # Using picsum.photos for random 224x224 images
        url = f"https://picsum.photos/224/224?random={i}"
        filepath = os.path.join(output_dir, f"test_image_{i:03d}.jpg")
        
        try:
            # We add a small sleep to avoid rate limiting and ensure random variety
            time.sleep(0.05)
            urllib.request.urlretrieve(url, filepath)
            if (i+1) % 10 == 0:
                print(f"  [{i+1}/{count}] downloaded...")
        except Exception as e:
            print(f"  [Error] Failed to download image {i}: {e}")

if __name__ == "__main__":
    # Ensure we are in the assets directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    download_images(100)
    print("Download complete.")
