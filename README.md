# 簡易遠端工作站+聊天室

## 簡介
實作可遠端存取的工作站，具體內容包括server、自行實作的shell，以及重新包裝過的client。
可以直接使用nc連線至server，也可使用本專案提供的client連線，有更多功能可使用。

## 開發環境/工具
- 使用C++，在Linux開發。
- client使用poll處理server訊息和使用者輸入。

## Usage
使用```make```編譯檔案
- server: server需要root權限設定使用者身分，server預設port為8787
    ```sudo ./worm_server```
- client: 預設server port為8787
    ```./worm_client <user_name>@<server_IP>```
- upload/download file:
    - upload: ```./worm_cp <src_file> <user_name@server_IP>:<dst_file>```
    - download: ```./worm_cp <user_name@server_IP>:<src_file> <dst_file>```
- add_user.sh: 需要root權限在伺服器新增使用者
    ```sudo ./add_user.sh```
- rm_user.sh:  需要root權限在伺服器移除使用者
    ```sudo ./rm_user.sh```

## 預計實作功能
- [x] 支援多使用者同時連線（multi-process的多使用者server）。
- [x] 簡易使用者登入，登入後自動導向該使用者的home directory，並將root directory限制在指定區域，使用者不應也不能存取到該區域之外的任何物件。
- [x] 撰寫shellscript，供administrator新增/刪除使用者。
- [x] 權限設置，使用者不能存取或查看其他使用者的home directory。
- [x] 支援簡易Shell功能，包含使用/bin資料夾下的指令、cd、pipe、file redirection等基礎功能。
- [x] 歡迎訊息。
- [x] 命令列prefix，顯示使用者名稱及當前所在資料夾。
- [ ] 支援Makefile。
- [ ] 重新包裝ls，將user id對應到確切使用者名稱、加上顏色(網路傳輸會吃掉ls自帶的顏色)
- [ ] 包裝client，提供：
    - [x] 按下ctrl+C的換行並清除已輸入內容。
    - [ ] 按下上下鍵提供讀取先前指令。 -> 目前查到的做法需另外安裝library。
    - [ ] 按下左右鍵提供更改當前輸入。 -> 目前查到的做法需另外安裝library。
- [ ] 提供上傳/下載檔案功能
    - [x] 可上傳單一檔案並指定destination檔案名稱
    - [ ] 可上傳檔案至特定資料夾
    - [ ] 可上傳整個資料夾

## 已實作內容demo
### 內建指令
- setenv [var name] [value]：新增或變更環境變數。
- printenv [var name]：印出指定環境變數。
- cd [path]: 切換資料夾到指定路徑，路徑可以是絕對路徑或相對路徑。
- who: 列出當前線上所有使用者。
- exit：離開shell。
    
#### demo流程：
1. 測試setenv/printenv。
2. 切換目錄
   - prefix顯示
   - 輸入cd回到home directory
   - 使用者僅能查看包裝好的環境
3. 列出所有使用者
4. ctrl+c不退出、使用exit退出

![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/ed367387-7c5d-4b5b-82b1-bd411a5042f0)


### Ordinary pipe & File redirection
當file redirection和pipe衝突，將優先使用file redirection。
- 支援Linux常見的pipe功能。
    - e.g., ```cat hello.txt | wc```.
- 支援Linux常見的stdin redirection "<"。
    - e.g., ```some_execution < preset_input.txt```.
    - 如檔案不存在，將顯示錯誤訊息。
- 支援Linux常見的stdout redirection ">" 和 ">>"。
    - e.g., ```cat hello.txt > hello_copy.txt```, ```cat hello.txt >> append.txt```.

#### demo流程：
1. File redirection "<"
2. File redirection ">"
3. File redirection ">>"
4. pipe

![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/f2f0aa85-bd68-4a45-8e6d-c70dc872fdaa)

### 權限
#### demo流程
1. 使用者jacky嘗試存取使用者ww的home directory目錄
2. 使用者jacky嘗試存取使用者ww的檔案

![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/e19bbf21-abcd-4436-9808-a5f0161de0c5)

### Upload/Download files
#### demo流程
1. 上傳檔案
2. 下載檔案

![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/eb0731fb-287e-4b24-b829-8a9c20bd4496)
