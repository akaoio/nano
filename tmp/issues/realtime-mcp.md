THIẾT KẾ STREAM REALTIME MCP PROTOCOLS

LƯU Ý: Doc này là ý tưởng thiết kế việc giao tiếp giữa NANO và các client sẽ gọi nó (trong tương lai) chứ không phải giao tiếp giữa IO và NANO.

# BỐI CẢNH

Bản chất của giao thức MCP là dựa trên JSON RPC 2.0. Mà bản chất của JSON RPC là sạng async là không hỗ trợ stream dữ liệu thời gian thực về cho MCP Client.

Khi MCP Client (đang là các test) gửi request tới MCP Server (tức là NANO, qua http hoặc ws), thì MCP Server làm sao để truyền các "chunk" về cho Client?

Client: là client trong tưởng tượng, nó sẽ là gọi tới NANO
Server: là NANO, nó giao tiếp với client qua các transport, nhưng tất cả đều theo chuẩn MCP protocol

# CÓ 1 CÁCH ĐỂ "STREAM" CHUNK VỀ CLIENT QUA HTTP

Các bước:
1. Client gửi request tới server theo đúng giao thức MCP, nhưng trong params bao gồm param stream: true.
2. Server nhận request như thường:
  - Nếu không thấy stream = true, hoặc thấy stream = false -> server trả kết quả thẳng sau khi tính toán
  - Nhưng nếu thấy param stream = true -> server tạo 1 id unique cho stream rồi trả stream id về cho client
3. Client nhận được stream id -> gửi lại request đúng method cũ, param chỉ cần có stream id
  - Mỗi lần client gửi request -> server trả về các đoạn chunk mà chưa trả
  - Mỗi lần server trả các đoạn chunk, nó sẽ giải phóng luôn các đoạn chunk đã trả khỏi bộ nhớ của nó
  - Mỗi chunk đều có seq để đánh thứ tự
  - Riêng chunk cuối cùng phải có end: true
  - Sau khi trả xong chunk cuối cùng (có end:true) thì server giải phóng hoàn tòan bộ nhớ liên quan tới đoạn stream vừa rồi
  - Sau khi client đã lấy chunk cuối cùng, thì nếu client quay lại lấy tiếp sẽ nhận được phản hồi "stream session not exists or expired"

# GIẢI PHÁP NÀY LÀ TRUYỀN STREAM REALTIME QUA HTTP TRÊN GIAO THỨC MCP

## Đối với Websocket thì chúng ta stream chunk từ MCP Server về cho MCP Client như thế nào?

1. Client gửi request tới server theo đúng giao thức MCP, nhưng trong params bao gồm param stream: true.
2. Server nhận request như thường:
  - Nếu không thấy stream = true, hoặc thấy stream = false -> server (nano) trả kết quả thẳng sau khi (IO) tính toán
  - Nhưng nếu thấy param stream = true -> server tạo 1 id cho stream
3. Server trả thẳng từng chunk cho Client, mà không cần đợi Client pull về:
  - Mẫu chunk mà server gửi:
  ```json
  {
    "jsonrpc": "2.0",
    "method": "method_that_client_requested", // Method phải là tên method mà client đã gưỉ trước đó, chứ ko được là "stream" vì bản chất chúng ta đang stream nên không cần khẳng định
    "params": {
        "stream_id": "1s2d3xa5b", // Random a-z0-9, unique and persistent stream id
        "seq": 0, // Số tăng dần để client nối các chunk
        "delta": "Hello", // Nội dung chunk
        "end": false // false có nghĩa chưa stream xong
    }
  }
  ```

# GIỮA IO VÀ NANO GIAO TIẾP STREAM NHƯ NÀO?

IO về cơ bản là 1 cái dual queue, nó nhận queue đầu vào, cho vào rkllm xử lý, rồi trả kết quả về queue đầu ra cho nano.

- Cần nghiên cứu kỹ hệ thống xem hiện nay NANO đang thực sự vào queue đầu ra của IO để lấy dữ liệu, hay chỉ là lý thuyết?

- Hàm callback truyền vào cho rkllm để nhận đầu ra của rkllm đang ở IO hay ở NANO?

- RKLLM gọi callback thì IO cho vào queue, hay NANO đang tự ý vào lấy dữ liệu?

- Cần thực hiện nghiên cứu kỹ hạ tầng hệ thống để thiết kế kế hoạch triển khai ý tưởng này.

- Kiến trúc hiện tại của chúng ta đủ để cho IO truyền stream về cho NANO chưa?

Khảo sát rồi --> tạo file tmp/researches/analysis.md