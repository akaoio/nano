Đây là hệ thống MCP server bọc thư viện rkllm, giao tiếp đa kênh, stream thời gian thực. Hệ thống này ánh xạ toàn bộ hàm, params, hằng số, biến số, states của lib rkllm do rockchip cung cấp (C language) sang giao thức MCP (model context protocol) (jsonrpc 2.0), chạy multi transport (stdio, udp, tcp, http, ws) servers song song làm các kênh giao tiếp. Dù dùng transport nào, thì cũng chung 1 chuẩn MCP protocol.

- Tất cả transport đều có thể chạy đồng thời thông qua các cổng (port) khác nhau.
- User toàn quyền tùy biến hệ thống thông qua settings.json tự sinh khi chạy server. Các tùy biến này được ưu tiên hơn các cài đặt mặc định được định nghiã trong code của server (các cài đặt này về sau nên cho ra 1 file header).

Hệ thống này cho phép client toàn quyền sử dụng tất cả chức năng và trạng thái states mà rkllm cung cấp thông qua jsonrpc (MCP Protocol). 

Client có thể là:
- 1 agent khác
- Thậm chí là 1 isntance khác của server này
- 1 phần mềm viết bằng bất kỳ ngôn ngữ nào, bất kỳ môi trường nào
==> miễn là nó tương tác được qua stdio/tcp/udp/http/ws

# THIẾT KẾ STREAM REALTIME MCP PROTOCOLS

Đây là ý tưởng thiết kế việc giao tiếp giữa server và các client sẽ gọi nó.

## THỰC RA KHI NÀO MỚI CẦN STREAM DATA

Khi nào cần stream? Thực ra stream chỉ cần khi gọi lệnh rkllm_run_async trong lib rkllm.

## BỐI CẢNH

Bản chất của giao thức MCP là dựa trên JSON RPC 2.0. Mà bản chất của JSON RPC là không chú trọng hỗ trợ stream dữ liệu thời gian thực về cho MCP Client.

Khi MCP Client (đang là các test) gửi request để chạy method rkllm_run_async tới MCP Server (qua 1 trong các transport), thì MCP Server làm sao để truyền stream các "chunk" về cho Client?

Client: là client trong tưởng tượng (hiện đang là các test), nó sẽ là gọi tới server.
Server: giao tiếp với client qua các transport, nhưng tất cả đều theo chuẩn MCP protocol. Có 1 MCP Layer để mở gói và đóng gói đầu vào đầu ra.

# CÓ 1 CÁCH ĐỂ "STREAM" CHUNK VỀ CLIENT QUA BẤT KỲ TRANSPORT NÀO

## ĐỐI VỚI CÁC TRANSPORT STDIO, TCP, UDP, WS:

Các giao thức này về cơ bản thuộc dạng realtime, tức là sau khi thiết lập kết nối, một bên có thể gửi tin cho bên còn lại --> rất dễ để triển khai stream data.

1. Client chỉ cần gửi request theo đúng chuẩn MCP. Chỉ cần gọi method rkllm_run_async thì server tự biết là chuẩn bị stream.
2. Server gọi hàm tương ứng (rkllm_run_async) trong rkllm.
3. Mỗi khi rkllm gọi trả về token thì nó sẽ gọi hàm callback mà server đăng ký trước đó.
4. Hàm callback này chuyển dữ liệu tới MCP layer để đóng gói thành dạng string json tương thích với MCP protocol chuẩn rồi gửi tới transport tương ứng.

## ĐỐI VỚI HTTP TRANSPORT:

1. Client chỉ cần gửi request theo đúng chuẩn MCP. Chỉ cần gọi method rkllm_run_async thì server tự biết là chuẩn bị stream.
2. Server gọi hàm tương ứng (rkllm_run_async) trong rkllm.
3. Mỗi khi rkllm trả về token thì nó sẽ gọi hàm callback mà server đăng ký trước đó.
4. Hàm callback này chuyển dữ liệu tới 1 vùng đệm (dành riêng cho http transport) để xâu chuỗi các token lại để client vào poll. Toàn bộ dữ liệu được xâu chuỗi ở đây coi như 1 chunk.
5. Do trên môi trường http không stream realtime được, nên client phải dùng request id (đã dùng trong bước trước) để cào dữ liệu về, tạo ra hiệu ứng stream thật.
6. Mỗi khi client gửi yêu cầu với đúng method "poll" và request id cũ và params trống, thì các dữ liệu tại vùng đệm nói trên được chuyển tới MCP layer để đóng gói thành dạng string json tương thích với MCP protocol (jsonrpc 2.0) rồi gửi tới transporter (http) để trả về cho client.
7. Dữ liệu mới vẫn tiếp tục chảy vào vùng đệm và chưa qua MCP layer. Chỉ khi client poll thì dữ liệu mới được xả qua MCP layer rồi trả về transport http
8. Cứ mỗi vòng poll, sau 30 giây kể từ token đầu (hoặc kể từ lúc poll trước) mà client không vào poll thì vùng đệm tự xả để giải phóng bộ nhớ.
9. Vùng đệm tự giải phóng sau mỗi lần client poll data.

Cách này (cho http) khác với cách dành cho stdio tcp udp ws ở chỗ các transport kia ko cần vùng đệm. Còn http cần 1 vùng đệm để lưu các phản hồi của
model trong khi client chưa kịp poll, vì dữ liệu được server bắn trực tiếp tới client luôn.

## LƯU Ý:

- Mỗi chunk đều có seq để đánh thứ tự
- Riêng chunk cuối cùng phải có end: true
- Sau khi trả xong chunk cuối cùng

## MẪU DỮ LIỆU

### 1. Mẫu request từ client gửi để gọi method:
```json
{
	"jsonrpc": "2.0",
	"id": 123, // "id" này rất quan trọng vì dùng trong phản hồi từ server về cho client, chỉ cần dùng "id" là khỏi cần phải dùng stream id riêng (vì "id" cũng là unique trong từng cuộc gọi rồi)
	"method": "rkllm_run_async",
	"params": ...
}
```

### 2. Mẫu request client gửi để poll chunks từ server nếu đang dùng transport http:
```json
{
	"jsonrpc": "2.0",
	"id": 123,
	"method": "poll",
	"params": {}
}
```

### 3. Mẫu response thường từ server (nếu method mà client gọi không cần stream):
```json
{
	"jsonrpc": "2.0",
	"id": 123,
	"result": {
		// data trong này như bình thường
	 }
}
```

### 4. Mẫu chunk mà server gửi khi cần stream về cho stdio tcp udp websocket:
```json
{
	"jsonrpc": "2.0",
	"method": "method_that_client_requested", // Method phải là tên method mà client đã gưỉ trước đó, chứ ko được là "stream" vì bản chất chúng ta đang stream nên không cần khẳng định"
	"id": 123,
	"result": {
		"chunk": {	
			"seq": 0, // Số tăng dần để client nối các chunk
			"delta": "Hello", // Nội dung chunk (mỗi chunk là 1 token)
			"end": true // xuất hiện nếu đó là chunk cuối (vì đã có "seq" nên seq 0 là chunk đầu, nên không cần start true, chỉ cần end true)
		}
	 }
}
```

### 5. Mẫu chunk mà client nhận được khi poll nếu dùng http transport:
```json
{
	"jsonrpc": "2.0",
	"method": "method_that_client_requested", // Method phải là tên method mà client đã gưỉ trước đó, chứ ko được là "stream" vì bản chất chúng ta đang stream nên không cần khẳng định"
	"id": 123,
	"result": {
		"chunk": {	
			"seq": 0, // Số tăng dần để client nối các chunk
			"delta": "Hello friend, how", // Nội dung "chained" chunk (mỗi chunk bản chất là 1 đoạn nối sẵn của nhiều token, độ dài tùy vào interval time của client)
			"end": true // xuất hiện nếu đó là chunk cuối (vì đã có "seq" nên seq 0 là chunk đầu, nên không cần start true, chỉ cần end true)
		}
	 }
}
```

# HOW THE DATA FLOW FROM END TO END?

1. Client connects to Server using one of the provided transport (stdio, tcp, udp, http, ws)
2. Client send request jsonrpc request to server transport
3. Server transport pass incomming message to MCP layer
4. MCP layer parse the MCP json string -> pass to rkllm proxy
5. Rkllm proxy call targeted function or constants (whatever) on original rkllm lib
6. Rkllm lib calls callback function which is a buffer manager
7. Buffer manager check if transport is http and this is streaming session -> keep joining the tokens until client polls. If transport is not http, pass data to MCP layer to pack it to be MCP compliant
8. MCP layer packs message to make it MCP compliant -> then pass to the right transport
9. The transport sends MCP compliant packed json string back to client.