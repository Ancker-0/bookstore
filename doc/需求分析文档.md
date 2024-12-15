## 身份

用户可以具有下述身份之一：游客（guest），顾客（customer），销售人员（clerk），店长（host）。

每个身份具有执行相应操作的权限。

+ *游客* ：用户创建
+ *顾客* ：查询图书、购买图书
+ *销售人员* ：进货、图书信息录入、图书信息修改、创建顾客
+ *店长* ：查询图书、购买图书、进货、图书信息录入、图书信息修改、*用户创建？* 、查询采购信息、查询销售情况、查询盈利信息、查看员工工作报告、查看系统日志

## session

账号登陆后会将 session 保存在 `[session-file]` 中，作为访问该账号的凭据。可以通过 `--logout [username]` 注销。

`[session-file]` 可以保存多个账号的登陆状态，且有一个当前账号，当前账号可以通过 `--user [username]` 切换。

## 命令行参数

`./bookstore [option] ... [ [command] [option] ... ]`

### 账号相关

#### `--session-file [session-file]`

指定 session 文件，默认值为 `session.txt`

#### `--user [username] --password [password]`

若 `[password]` 存在，则以用户名 `[username]` 和密码 `[password]` 登陆系统，登陆后 session 保存在 `[session-file]`，并将 `[username]` 作为当前账号。

若 `[password]` 不存在，则在 `[session-file]` 里寻找对应 `[username]` 的 session，并将其作为当前账号。

#### `--list-session`

列出所有 session 和当前账号。

#### `--logout`

登出当前用户以及对应 session。

#### `--logout-session [username]`

登出 `[username]` 以及对应 session。

#### `--register [username] [password] [identity]`

注册账号，`[identity]` 是 `guest`、`custormer`、`clerk`、`host` 之一。

### 其他

#### `--date [date]`

设定操作的日期，格式为 YYYY-MM-DD。

## 命令

这里列出 `./bookstore [option] ... [ [command] [option] ... ]` 中可能的 `[command]`。

### `query-book [query-sexp]`

查询满足条件的书。

`[query-sexp]` 是一个 S-表达式，定义如

```
<query-sexp>  ::=  (and <query-sexp> ...)    |
                   (or <query-sexp> ...)     |
                   (equal <expr> <expr>)     |
                   (contain <expr> <expr>)

<expr>        ::=  (isbn)                    |
                   (author)                  |
                   (keyword)                 |
                   (name)                    |
                   <string>
```

### `buy [ISBN] [n]`

按 ISBN 号购买 n 本书。

### `add-book-info [ISBN] [bookname] [author] [number] [price] [keywords] ...`

添加图书。

### `import-book [ISBN] [n]`

图书进货。进货前需保证图书存在。

### `import-log --begin [begin-date] --end [end-date]`

查询从 `[begin-date]` 到 `[end-date]` 期间的进货记录。

### `sell-log --begin [begin-date] --end [end-date]`

查询从 `[begin-date]` 到 `[end-date]` 期间的售货记录。

### `operate-log --begin [begin-date] --end [end-date] --user [username]`

查询用户 `[username]` 从 `[begin-date]` 到 `[end-date]` 期间的操作记录。

### `profit-log --begin [begin-date] --end [end-date]`

查询从 `[begin-date]` 到 `[end-date]` 期间的盈利情况。

### `system-log --begin [begin-date] --end [end-date]`

查询从 `[begin-date]` 到 `[end-date]` 期间的系统日志。
