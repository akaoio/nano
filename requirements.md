Tôi muốn viết một bộ phận để bọc lấy lib của hãng cho tại:
- src/libs/rkllm/rkllm.h
- src/libs/rkllm/librkllmrt.so

Tôi muốn dùng C23 cho hiện đại.
Tôi muốn ánh xạ toàn bộ các hàm của rkllm và loại bỏ toàn bộ "rkllm_" prefix.

Bộ phận này gọi là io, nó là cổng giao tiếp duy nhất tới rkllm
Bản thân nó là 1 cái queue, để nó có thể phục vụ nhiều callers, và để tránh crash. Bộ phận io là đứng riêng, nó như phần lõi hệ thống. io chỉ đóng vai trò là một cái queue và là bộ phận truyền dẫn các request và response, chứ bản thân nó không tính toán. Nó ánh xạ toàn bộ hàm trong rkllm và chỉ bỏ đi "rkllm_" ở đầu.

Tiếp theo là bộ phận "nano". "nano" cũng là trên của project này.
"nano" làm nhiệm vụ gì? Nó đứng giữa các client và io. Nó cho phép client gọi tới thông qua transporter UDP, TCP, HTTP, WS, STDIO. Và tất cả transporter đều theo chuẩn giao thức MCP.
Khi nano chạy lên nó sẽ chạy tất cả các giao thức, coi như là một lớp interface tổng.

# VẤN ĐỀ THÁCH THỨC KỸ THUẬT

MCP bản chất là JSON, không phải C++. Mà các hàm của rkllm đôi khi cần LLMHandle*. Vậy thì ta phải ánh xạ LLMHandle bằng id, để Client có thể dùng id truyền thới nano trong các request cần thiết.

Chúng ta phải thiết kế làm sao để số dòng code là tối thiểu, và hiệu quả đạt được phải tối đa.

# CÁC FILE CHO TRƯỚC:
models/qwenvl/model.rkllm (file model Qwen 2.5 VL)
models/lora/lora.rkllm và model.rkllm (để test các chức năng liên quan đến lora)
Không được gọi các file này từ trong hardcode, chỉ dùng trong unit test để truyền tới io.

tôi muốn io nó không thực hiện tính toán gì, nó chỉ queue rồi cast thẳng request tới rkllm rồi phản hồi. Phản hồi thì cũng phải qua queue chứ không phản hồi thẳng về caller. caller ở đây là nano. Khi phản hồi thì nó đã phải kèm theo cả id của LLMHandle, còn nano chỉ vào lấy phản hồi từ queue (đã kèm id của LLMHandle) và phản hồi lại cho Client theo đúng transporter tương ứng. Nói chung, io phải tự quản tất cả mọi thứ. Nó như 1 bộ phận inference, queue, nhận lệnh từ queue và chạy, rồi phản hồi ra queue. 

MỘT SỐ VẤN ĐỀ LIÊN QUAN

Trong các máy tính có phần cứng mạnh (như Orange Pi 5 Plus với 32GB RAM) thì có thể chạy tới 2 hoặc 3 model như @/models/qwenvl/model.rkllm song song, vì vậy cần thiết kế io làm sao để nó có thể chạy và quản lý song song nhiều LLMHandles mà không bị gặp các sự cố, đặc biệt các sự cố về memory.

Cần phải hạn chế tối đa số dòng code. Nhưng quan trọng nhất là phải đạt được hiệu quả tối đa. Vì khi code ngày càng nhiều, chi phí duy trì, chi phí quản lý và phát triển rất cao.

Phương pháp triển khai là không được mockup, mọi thứ phải dùng thật luôn. Làm đến đâu là đạt được thành tựu chắc chắn đến đó chứ không được để lại các nợ kỹ thuật.

Mọi thành phần trong code đều phải build riêng được, test riêng được, thì mới phục vụ quá trình dev nhanh chóng, đỡ mất thời gian build lại toàn bộ.

Phần io hiện tại là phần cần được chú trọng nhiều nhất. Vì nó chính là trái tim của hệ thống. Qua nó mà mọi thứ giao tiếp với rkllm. Nhưng io không chứa logic, nó sẽ phải được thiết kế làm sao mà ánh xạ một cách ngắn gọn và thông minh nhất tới rkllm. Nó chỉ như 1 người phục vụ nhà hàng, nó nhận request từ nano, rồi nó tìm llmhandle tương ứng, rồi nó cho vào queue để rkllm xử lý, sau đó nó lại cho vào queue để nano vào lấy. Lưu ý: nano chỉ biết id của llmhandle chứ không thực sự quản lý llmhandle.

CÁC MODELS ĐỂ TEST

@/models/qwenvl/ chứa model để chạy các test cho model thường
@/models/lora chứa model kiểu lora để test các chức năng liên quan đến lora

TREE HIỆN TẠI

x@orangepi5-plus:~/Projects/nano$ tree
.
├── build.sh
├── LICENSE
├── models
│   ├── lora
│   │   ├── lora.rkllm
│   │   └── model.rkllm
│   └── qwenvl
│       └── model.rkllm
├── nano
├── plan.md
├── requirements.md
└── src
    ├── libs
    │   └── rkllm
    │       ├── librkllmrt.so
    │       └── rkllm.h
    └── main.c

7 directories, 11 files