# Simple-remote-workstation&chatroom
A concurrent multi-process server with chat room.
(整改中)
## 功能
  - 支援多使用者同時連線（實作multi-process多使用者server）。  
  - 為每一位使用者建立一個新的process處理所有request。  
  - 支援聊天室功能，包含更改暱稱、私訊、廣播訊息和傳遞檔案等功能（請參考下方有關聊天室功能的區塊）。  
  - 運用shared memory處理使用者聊天室訊息。  
  - 運用FIFO處理使用者之間傳遞的檔案。  
  - Usage ```./multi_proc_server [port]```
  - 連線方式：```nc [ip] [port]```
  
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
