# 1. 파이썬 환경 체크 및 라이브러리 설치
Write-Host "--- 1. Installing Python Libraries ---" -ForegroundColor Cyan
python -m pip install torch torchvision onnx onnxscript

# 2. 파이토치 모델 추출 (Python 실행)
Write-Host "--- 2. Exporting PyTorch to ONNX ---" -ForegroundColor Cyan
$env:PYTHONIOENCODING = "UTF-8"
python export_model.py

# 3. ncnn 도구 다운로드 (없을 경우에만)
$ncnn_zip = "ncnn-20240410-windows-vs2022.zip"
$ncnn_url = "https://github.com/Tencent/ncnn/releases/download/20240410/$ncnn_zip"
$ncnn_dir = "ncnn_tools"

if (-not (Test-Path "$ncnn_dir\bin\onnx2ncnn.exe")) {
    Write-Host "--- 3. Downloading ncnn tools ---" -ForegroundColor Cyan
    Invoke-WebRequest -Uri $ncnn_url -OutFile $ncnn_zip
    Expand-Archive -Path $ncnn_zip -DestinationPath $ncnn_dir -Force
    Remove-Item $ncnn_zip
}

# 4. ncnn 변환 실행
Write-Host "--- 4. Converting ONNX to ncnn ---" -ForegroundColor Cyan
$onnx2ncnn = ".\$ncnn_dir\ncnn-20240410-windows-vs2022\x64\bin\onnx2ncnn.exe"
& $onnx2ncnn efficientnet_b0.onnx efficientnet_b0.param efficientnet_b0.bin

# 5. 결과 확인
if (Test-Path "efficientnet_b0.param") {
    Write-Host "`nSuccessfully generated:" -ForegroundColor Green
    Write-Host " - efficientnet_b0.param" -ForegroundColor Green
    Write-Host " - efficientnet_b0.bin" -ForegroundColor Green
    Write-Host "`nYou can now use these files in your Vulkan project!" -ForegroundColor Green
} else {
    Write-Host "`nConversion failed. Please check the errors above." -ForegroundColor Red
}
