# Simple-remote-workstation&chatroom
A concurrent multi-process server with chat room.
(整改中)
## 預計實作功能
- 支援多使用者同時連線（multi-process的多使用者server）。
    - Main server：處理新連線請求和使用者聊天訊息。
    - Data server：處理使用者檔案傳輸。
- 為每一位使用者建立一個新的process處理所有request。
    - 與main server建立連線，接收/發送聊天室訊息。
    - 與Data server建立連線，要求/發送其他使用者傳輸檔案。
    - 與遠端使用者建立連線，處理使用者發送的指令。
- 支援簡易Shell功能，包含使用指定資料夾下的指令、pipe、stdin/stdout redirection等（請參考下方Shell功能區塊）。
- 支援聊天室功能，包含更改暱稱、私訊、廣播訊息和傳遞檔案等功能（請參考下方聊天室功能區塊）。
- Usage ```./server [port]```
- 使用nc連線測試server，連線方式：```nc [server ip] [port]```

## Shell功能
- 內建指令
  - setenv [var name] [value]：新增或變更環境變數。
  - printenv [var name]：印出指定環境變數。
  - exit：離開shell。
- 指令資料夾下的執行檔
  - 於開啟server時將指定資料夾路徑作為參數傳入。
      - e.g., ```./server /usr/bin```.
  - 使用者只能執行指定資料夾下的指令，不受系統中其他執行檔影響。
  - Ordinary pipe 
  - 支援Linux常見的pipe功能。
      - e.g., ```cat hello.txt | wc```
- File redirection
  - 支援Linux常見的stdin redirection功能。
      - e.g., ```some_execution < preset_input.txt```.
      - 如檔案不存在，將顯示錯誤訊息。
  - 支援Linux常見的stdout redirection功能。
      - e.g., ```cat hello.txt > hello_copy.txt```.
      - 如檔案已存在，將複寫檔案。

## 聊天室功能
下面使用三個使用者的情境demo聊天室功能
- 歡迎訊息和離開訊息
    - 若有使用者的加入，發送歡迎訊息並廣播通知所有其他使用者。
    - 若有使用者離開，廣播通知所有使用者。
    
	![image](https://user-images.githubusercontent.com/96563567/225505193-d5f92cc3-9827-422d-a2a4-a258384bc146.png)  
	---
	![image](https://user-images.githubusercontent.com/96563567/225505227-dd69f192-45dc-4cad-aecb-fb8f7782e6ac.png)
***

- who：列出線上所有使用者資訊，資訊包括ID、暱稱、使用者IP:port

	![image](https://user-images.githubusercontent.com/96563567/225500798-feed582f-0dba-4034-be8d-8a9d305510ad.png)  
	![image](https://user-images.githubusercontent.com/96563567/225500861-e257ed21-f8da-4431-a7fa-adb3a97fe953.png)  
***

- name [new name]：更改暱稱。
    - 當使用者更改暱稱，廣播通知所有使用者。  
 	  
  ![image](https://user-images.githubusercontent.com/96563567/225503514-3befe42a-ce25-42bf-a82d-7a8559883834.png)
***

- tell [user id] [msg]：私訊特定使用者  
- yell [msg]： 廣播訊息  

  ![image](https://user-images.githubusercontent.com/96563567/225504397-513a46b6-4dbf-4fff-a61b-f9edee25ddd3.png)
***

- \>[user id]/\<[user id]：使用者之間發送/接收檔案
    - 發送檔案到公共區域：
        - 所有使用者皆可依檔名存取公共區域檔案。
        - 檔案不隨使用者離開而消失。
    - 發送檔案給其他使用者：
        - 傳送檔案給不存在/已離開的使用者將顯示錯誤訊息。
        - 傳送不存在的指令將顯示錯誤訊息。
        - 讀取不存在的檔案可以成功發送，相關錯誤訊息將在接收方印出。
    - 接收其他使用者傳送的檔案：
        - 接收來自不存在/已離開的使用者發送的檔案將顯示錯誤訊息。
        - 若目標使用者尚未傳送檔案給當前使用者，顯示錯誤訊息。
    - 當使用者成功傳送或接收檔案，廣播通知所有使用者。
    
    ![image](https://user-images.githubusercontent.com/96563567/225506375-9450d8c8-f0ab-4637-ad39-54a3ff3e0fa7.png)
    ---
    ![image](https://user-images.githubusercontent.com/96563567/225505623-4f27a3eb-3000-4e87-8a06-d13b10aa1dd6.png)