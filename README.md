# Remote-Working-Ground-Server
實作了三種不同形式的遠端工作站  
## Concurrent connection-oriented server  
  - 支援單一使用者連線，工作站詳細內容可以參考我的另一個repo：https://github.com/luckyjp6/Shell。  
  ![image](https://user-images.githubusercontent.com/96563567/225498300-5c16313c-ef90-4342-8a4f-5738c64ccba7.png)  
  
## Concurrent single-process server with chat room  
  - 支援多使用者同時連線（實作單一process多使用者server）。  
  - 運用select函數控管新進連線或已存在連線用戶的request。  
  - 支援聊天室功能，包含更改暱稱、私訊、廣播訊息和傳遞檔案等功能（請參考下方有關聊天室功能的區塊）。  
  
## Concurrent multi-process server with chat room  
  - 支援多使用者同時連線（實作multi-process多使用者server）。  
  - 為每一位使用者建立一個新的process處理所有request。  
  - 支援聊天室功能，包含更改暱稱、私訊、廣播訊息和傳遞檔案等功能（請參考下方有關聊天室功能的區塊）。  
  - 運用shared memory處理使用者聊天室訊息。  
  - 運用FIFO處理使用者之間傳遞的檔案。  
  
## 聊天室功能
下面使用三個使用者的情境demo聊天室功能
### 歡迎訊息和離開訊息（使用者的加入和離開將廣播給所有使用者）
  ![image](https://user-images.githubusercontent.com/96563567/225505193-d5f92cc3-9827-422d-a2a4-a258384bc146.png)  
  ---
  ![image](https://user-images.githubusercontent.com/96563567/225505227-dd69f192-45dc-4cad-aecb-fb8f7782e6ac.png)
***

### who
  - 列出線上所有使用者訊息  
  ![image](https://user-images.githubusercontent.com/96563567/225500798-feed582f-0dba-4034-be8d-8a9d305510ad.png)  
  ![image](https://user-images.githubusercontent.com/96563567/225500861-e257ed21-f8da-4431-a7fa-adb3a97fe953.png)  
***

### name [new name]
  - 更改暱稱  
  - 將廣播更改暱稱資訊  
  ![image](https://user-images.githubusercontent.com/96563567/225503514-3befe42a-ce25-42bf-a82d-7a8559883834.png)
***

### tell [user id] [msg]
  - 私訊特定使用者  
### yell [msg]
  - 廣播訊息  
  ![image](https://user-images.githubusercontent.com/96563567/225504397-513a46b6-4dbf-4fff-a61b-f9edee25ddd3.png)
***

### \>[user id]/\<[user id]：使用者之間發送/接收檔案（將廣播發送和接收檔案資訊）  
  - ＂>n＂發送檔案id為n的使用者  
    - 傳送檔案給不存在的使用者將顯示錯誤訊息
    - 使用不存在的指令將顯示錯誤訊息
    - 讀取不存在的檔案可以成功發送，相關錯誤訊息將在接收方印出
  - ＂<n＂接收來自id為n的使用者傳送的檔案  
    - 接收來自不存在的使用者發送的檔案將顯示錯誤訊息
    - 若id為n的使用者尚未傳送檔案給當前使用者將顯示錯誤訊息
    
    ![image](https://user-images.githubusercontent.com/96563567/225506375-9450d8c8-f0ab-4637-ad39-54a3ff3e0fa7.png)
    ---
    ![image](https://user-images.githubusercontent.com/96563567/225505623-4f27a3eb-3000-4e87-8a06-d13b10aa1dd6.png)
