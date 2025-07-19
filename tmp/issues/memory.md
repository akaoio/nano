VẤN ĐỀ VỀ BỘ NHỚ

Khi bắt đầu dự án này tôi cứ ngỡ rằng sẽ chạy được nhiều model song song trong rkllm, vì tôi thấy lib nó chứa LLMHandle. Nhưng thực tế thì không phải. Tuy lib hãng cung cấp có LLMHandle, nhưng thực tế chip RK3588 có vẻ không hỗ trợ chạy song song nhiều model. 

Tôi đã thí nghiệm gọi rkllm_init để load và run 2 models. Kết quả thật thú vị, nếu load và run 2 model cỡ nhỏ thì chúng load và phản ứng bình thường. Nhưng khi load model lớn và model nhỏ thì nhỉ load được model nhỏ.

Có lúc thì nếu load thành công, khi thử "hello" chúng sẽ phản hồi nội dung hỗn loạn vô nghĩa.
Nhưng có lúc thì cả 2 đều phản hồi nội dung có ý nghĩa.

Tôi cũng đã thử reverse engineer librkllmrt.so. Quá trình reverse engineer cho thấy có vẻ như rkllm cộng thêm một chút bộ nhớ làm vùng đệm khi load model. Vùng đệm đó chính là lý do mà tôi có thể load được model nhỏ sau khi đã load model lớn.

Sự thật này cho thấy dẫn đến việc kiến trúc hiện tại của IO đã "over-engineer" so với khả năng đáp ứng của lib rkllm (và có thể là cả phần cứng).

Kiến trúc của IO có ánh xạ handle id để quản lý LLMHandle pointers, và worker pool, được thiết kế để chạy song song models. Nhưng nếu phần cứng không hỗ trợ chạy song song nhiều models, thì chúng ta có nên lược bỏ không? Hoặc không lược bỏ nhưng setup xuống 1 worker thread duy nhất và chỉ load 1 model duy nhất? Đợi sau này lib hãng cho phép thì ta mở lại nhiều worker threads hơn?

Nếu loại bỏ worker pool và handle id thì dòng chảy dữ liệu sẽ nên diễn ra như nào?
Tôi có đang hiểu sai về worker pool không?

VIỆC QUẢN LÝ BỘ NHỚ NPU

RKLLM tự quản lý bộ nhớ, khi gọi rkllm_init thì các param truyền vào không thấy có nhắc gì đến việc xin cấp bộ nhớ bao nhiêu. Cả rkllm.h cũng cho thấy không có chỗ nào rõ ràng về việc xin cấp bộ nhớ cho NPU. Có vẻ như hãng không cho chúng ta can thiệp vào phần cứng. Khi load 1 model thì rkllm tự cấp bộ nhớ cho NPU dựa trên RAM, kích thước của model, và một vùng đệm khoảng 1-2 GB. Khi load model thứ 2 thì rkllm bị crash vì không đủ memory.

Việc load model 2 trong khi model 1 đã được load sẽ gây ra crash cả model 1. Chúng ta cần phải có kế hoạch để hệ thống không bị crash.

Cần nghiên cứu kỹ lưỡng, khảo sát kiến trúc của hệ thống rồi viết vào tmp/researches/memory-analysis.md