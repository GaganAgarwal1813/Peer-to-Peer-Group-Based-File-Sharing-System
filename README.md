# Advance Operating System Assignment - 3

## Peer-to-Peer Group Based File Sharing System

### Tracker :
Compile Tracker : 
`g++ tracker.cpp -o tracker -pthread` 

Run Tracker : 
`./tracker​ <TRACKER INFO FILE> <TRACKER NUMBER>`

Example : 
``./tracker tracker_info.txt 1``


### Client :
Compile Client : 
`g++ client.cpp -o client -pthread -lcrypto` 

Run Client : 
`./client​ <IP>:<PORT> <TRACKER INFO FILE>`

Example : 
``./client 127.0.0.1:18000 tracker_info.txt``


#### tracker_info.txt : 

It conatains the details of trackers -
- IP address of both the Trackers 
- Port of both the Trackers 

Example :
``` 
    127.0.0.1
    5000
    127.0.0.1
    6000

```

### Commands :

- Create user account :
```
   create_user​ <user_id> <password> 
```
- Login :
```
    login​ <user_id> <password>
```
- Create Group :
```
    create_group <group_id>
```
- Join Group :
```
    join_group​ <group_id>
```
- Leave Group :
```
    leave_group​ <group_id>
```
- List pending requests :
```
    list_requests ​<group_id>
```
- Accept Group Joining Request :
```
    accept_request​ <group_id> <user_id>
```
- List All Group In Network :
```
    list_groups
```
- List All sharable Files In Group :
```
    list_files​ <group_id>
```
- Upload File :
```
    upload_file​ <file_path> <group_id​>
```
- Download File :
```
    download_file​ <group_id> <file_name> <destination_path>
```
- Logout :
```
    logout
```
- Show_downloads :
```
    show_downloads
```
- Stop sharing :
```
    stop_share ​<group_id> <file_name>
```




