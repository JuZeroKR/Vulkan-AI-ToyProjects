import torch
import torchvision.models as models
import os

def export_efficientnet_b0():
    # 1. Load pretrained EfficientNet-B0 model from torchvision
    print("Loading pretrained EfficientNet-B0 model...")
    model = models.efficientnet_b0(weights=models.EfficientNet_B0_Weights.DEFAULT)
    
    # 2. Set the model to evaluation mode
    model.eval()

    # 3. Create a dummy input tensor for ONNX export
    # Our preprocessing pipeline resizes the image to 224x224.
    # The input shape should be (Batch, Channels, Height, Width).
    dummy_input = torch.randn(1, 3, 224, 224)

    # 4. Export the model to ONNX format
    onnx_filename = "efficientnet_b0.onnx"
    print(f"Exporting model to {onnx_filename}...")
    
    torch.onnx.export(
        model, 
        dummy_input, 
        onnx_filename, 
        export_params=True, 
        opset_version=11, 
        do_constant_folding=True, 
        input_names=['input'], 
        output_names=['output']
    )

    if os.path.exists(onnx_filename):
        print(f"Successfully exported {onnx_filename}")
        print("\nNext step: Convert ONNX to ncnn format using onnx2ncnn:")
        print(f"onnx2ncnn {onnx_filename} efficientnet_b0.param efficientnet_b0.bin")
    else:
        print("Failed to export ONNX file.")

if __name__ == "__main__":
    export_efficientnet_b0()
