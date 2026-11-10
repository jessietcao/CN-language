// program     → function* EOF
// function    → type (IDENTIFIER | 主函数) "（" "）" block
// block       → "{" statement* "}"
// statement   → varDecl | assignStmt | ifStmt | whileStmt
//             | returnStmt | outputStmt | inputStmt | block
// varDecl     → type IDENTIFIER ("=" expression)? "。"
// assignStmt  → IDENTIFIER "=" expression "。"
// ifStmt      → 如果 "（" expression "）" block (否则 (ifStmt | block))?
// whileStmt   → 当 "（" expression "）" block
// returnStmt  → 返回 expression? "。"
// outputStmt  → 输出 "（" expression "）" "。"
// inputStmt   → 输入 "（" IDENTIFIER "）" "。"

// expression  → equality
// equality    → comparison (("==" | "!=") comparison)*
// comparison  → term (("<" | "<=" | ">" | ">=") term)*
// term        → factor (("+" | "-") factor)*
// factor      → unary (("*" | "/") unary)*
// unary       → "-" unary | primary
// primary     → INT | DOUBLE | STRING | CHAR | 真 | 假 | IDENTIFIER | "(" expression ")"