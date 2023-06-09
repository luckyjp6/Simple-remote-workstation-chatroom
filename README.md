# 簡易遠端工作站

## 簡介
實作可遠端存取的工作站，具體內容包括server、自行實作的shell、給管理者用的小工具，以及重新包裝過的client和供client從工作站上傳/下載檔案的程式。
可以直接使用nc連線至server，也可使用本專案提供的client連線，有更多功能可使用。


## 開發環境/工具
使用C++，在Linux開發，除/bin, /dev, /lib, /lib64資料夾下的檔案之外，皆為自行實作的程式，並未使用/bin/sh或其他現成shell。

## Usage
- 使用```make```產生執行檔
- server: server需要root權限設定使用者身分，server預設port為8787
    ```sudo ./worm_server```
- add_user.sh: 需要root權限在伺服器新增使用者
    ```sudo ./add_user.sh```
- rm_user.sh:  需要root權限在伺服器移除使用者
    ```sudo ./rm_user.sh`
- client: 預設server port為8787
    ```./worm_client <user_name>@<server_IP>```
- upload/download file:
    - upload: ```./worm_cp <src_file> <user_name@server_IP>:<dst_file>```
    - download: ```./worm_cp <user_name@server_IP>:<src_file> <dst_file>`````

## 實作功能 （包含未來可實作功能）
- Server
    - [x] 支援多使用者同時連線（multi-process的多使用者server）。
    - [x] 簡易使用者登入（帳號&密碼），登入後自動導向該使用者的home directory，並將root directory設置在指定資料夾（userspace），使用者只能存取到該資料夾下的物件。
- Shell
    - [x] 支援內建指令，包括setenv, printenv, cd, who。
    - [x] 支援pipe, file redirection等功能。
    - [x] 可解析command中的環境變數。
    - [x] 歡迎訊息。
    - [x] 命令列prefix，顯示使用者名稱及當前所在資料夾。
    - [ ] 支援Makefile。
    - [ ] 重新包裝ls，將user id對應到確切使用者名稱、加上顏色(網路傳輸會吃掉ls自帶的顏色)
- 管理者小工具
    - [x] 撰寫shellscript，供administrator新增/刪除使用者。
    - [x] 權限設置，使用者不能存取或查看其他使用者的home directory。
- Client
    - [x] 協助輸入使用者名稱
    - [x] 按下ctrl+C的換行並清除已輸入內容。
    - [x] 輸入密碼時隱藏輸入
    - [ ] 清除螢幕功能
    - [ ] 按下上下鍵提供讀取先前指令。 -> 目前查到的做法需另外安裝library。
    - [ ] 按下左右鍵提供更改當前輸入。 -> 目前查到的做法需另外安裝library。
- [ ] 提供上傳/下載檔案功能
    - [x] 支援上傳單一檔案並指定destination檔案名稱
    - [ ] 支援上傳整個資料夾

## 專案架構
    Project
    └── codes
    └── user_space    -> root directory
        └── bin
        └── dev
        └── home
            └── ww
            └── jacky
        └── lib       -> 因重設root directory，需將library搬到指令可以看見的位置以執行指令
        └── lib64
        └── share     -> offering a directory for all the users to share their files
    └── Makefile
    └── add_user.sh
    └── rm_user.sh

## 已實作內容demo

### 管理者小工具
- 新增使用者包含：
    - 於系統中新增使用者
    - 將使用者加入worm_server群組
    - 建立使用者家目錄並處理相關權限設定
- 移除使用者包括：
    - 將使用者從系統中刪除
    - 清除使用者家目錄

![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/c730b91f-39a1-4aa0-b06f-94772d059e56)

### Server
- 支援多使用者同時連線
    - 以fork實作
- 簡易使用者登入（帳號&密碼）
    - 同時會檢查使用者是否屬於worm_server群組
    - 不存在的使用者  
        ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/0c33efb0-73f9-4d92-8946-53b78004e9ab)
    - 錯誤密碼  
        ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/7aa3c32c-a598-44fa-9520-311138069bcb)
    - 正常登入  
        ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/9a058765-8780-4ef7-a642-be32eaf2fdb2)
- 登入後自動導向該使用者的home directory
    - 以chdir實作
    - 以使用者ww為例
    
    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/ffd2bba9-f63b-4a97-b2d4-ede439780d1d)
- 使用者僅能存取userspace資料夾下的物件 
    - 以chroot實作
    - 不能取得userspace資料夾外的檔案或資料夾 
    
    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/b4b82c94-b276-4b7f-9437-70e227b7b41d)
    

### Shell
- 歡迎訊息
    - 包含當前線上人數和使用者名稱  
        ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/570dc108-0a2b-430c-b788-34b14e406345)

- 環境變數
    - setenv [var name] [value]：新增或變更環境變數。
        - 使用setenv實作
    - printenv [var name]：印出指定環境變數。
        - 使用getenv實作
    - 環境變數解析格式：```$[envname]``` 

    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/dcd92109-2829-4a62-8f4f-4f8366b53167)
- 路徑處理
    - cd
        - 使用chdir實作
    - Home directory處理
    - 命令列路徑prefix

    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/681cbe23-fe32-496b-a0ff-4d2574b1d2fe)
- 顯示線上使用者 
    - 使用share memory實作
    - ```who``` 
    
    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/2f1085cd-5224-47ca-81a0-1b8e1860d28c)
- Pipe, file redirection  
    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/fe4e065b-0b75-4417-a92c-00847b4ed20d)


### Client
- 協助輸入user name
- 隱藏密碼輸入
    - 關閉terminal echo
- Server斷線時自動關閉
- ctrl+C 不會關閉client端，且不影響後續指令輸入
    - 使用signal實作
- 自製client端：  
    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/9206e092-7b9c-4df2-a076-b163756416a1)
- nc對照組：  
    ![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/d2d2143b-d7bd-4603-89a1-a556e7666b34)


### Upload/Download files
- 上傳/下載檔案使用同一支程式，程式接收兩個參數，第一個是source，第二個是destination
- demo流程 （Steps 1~3為上傳）
    1. （R）確認工作站無檔案up.txt
    2. （L）確認up.txt檔案大小並上傳至工作站
    3. （R）工作站收到up.txt，且檔案大小相符
    5. （L）確認本地無檔案down.txt
    6. （L）從工作站下載檔案up.txt，更名為down.txt
    7. （L）使用diff確認檔案一致

![image](https://github.com/luckyjp6/Simple-remote-workstation-chatroom/assets/96563567/38ac6584-1051-432b-b9f3-0ba3f83979e4)



