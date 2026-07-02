
# SmartHome IoT ESP32 Firebase
Đề tài: Hệ thống nhà thông minh giám sát và cảnh báo thời gian thực sử dụng ESP32 và Firebase.

## Chức năng chính

- Đọc nhiệt độ và độ ẩm bằng DHT11
- Đọc giá trị khí gas bằng MQ-4
- Phát hiện chuyển động bằng PIR HC-SR501
- Gửi dữ liệu lên Firebase Realtime Database
- Hiển thị dữ liệu trên giao diện Web
- Điều khiển LED từ giao diện Web
- LED cảnh báo sáng khi phát hiện chuyển động

## Cấu trúc thư mục

- esp32: chứa code nạp cho ESP32
- web: chứa giao diện HTML, CSS, JavaScript
- firebase: chứa cấu trúc JSON mẫu
- images: chứa hình ảnh/icon dùng cho giao diện
