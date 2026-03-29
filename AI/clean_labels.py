import urllib.request

url = "https://raw.githubusercontent.com/pytorch/hub/master/imagenet_classes.txt"
try:
    with urllib.request.urlopen(url) as response:
        content = response.read().decode('utf-8')
        labels = content.splitlines()
        
        # Clean labels
        clean_labels = [l.strip() for l in labels if l.strip()]
        
        with open(r"AI\imagenet_classes.txt", "w", encoding="utf-8") as f:
            for label in clean_labels:
                f.write(label + "\n")
        
        print(f"Successfully wrote {len(clean_labels)} labels to AI/imagenet_classes.txt")
        print(f"Index 904 is: {clean_labels[904]}")
except Exception as e:
    print(f"Error: {e}")
