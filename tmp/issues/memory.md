VẤN ĐỀ VỀ BỘ NHỚ

Vấn đề 1:

Khi bắt đầu dự án này tôi cứ ngỡ rằng sẽ chạy được nhiều model song song trong rkllm, vì tôi thấy lib nó chứa LLMHandle. Nhưng thực tế thì không phải. Tuy lib hãng cung vấp có LLMHandle, nhưng thực tế chip RK3588 có vẻ không hỗ trợ init nhiều LLMHandle, mà chỉ được 1. Điều này cần được kiểm chứng.

Sự thật này dẫn đến việc kiến trúc hiện tại của IO không thực sự tối ưu cho chip RK35xx series.

Kiến trúc của IO có ánh xạ handle id để quản lý LLMHandle pointers, và worker pool, được thiết kế để chạy song song models. Nhưng nếu phần cứng không hỗ trợ chạy song song nhiều models, thì chúng ta nên lược bỏ. Hoặc không lược bỏ nhưng setup xuống 1 worker duy nhất? Đợi sau này lib hãng cho phép thì ta mở lại nhiều worker?

Nếu loại bỏ worker pool và handle id thì dòng chảy dữ liệu sẽ nên diễn ra như nào?

Cần phải thực hiện thí nghiệm để xác thực chuyện chip không hỗ trợ chạy song song. Vì nếu không đúng mà lại đi loại bỏ workers với handle id thì đúng là tự sát.



Vấn đề 2:

Khi đã cấp bộ nhớ cho NPU thì sau đó không thể thay đổi. Muốn thay đổi thì phải destroy rồi init lại? Điều này gây cho hệ thống mất sự linh hoạt. Cần tìm hướng giải quyết. Nên có nhiều hướng để lựa chọn.