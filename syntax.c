/* =========================== Syntax highlights DB =========================
 *
 * In order to add a new syntax, define two arrays with a list of file name
 * matches and keywords. The file name matches are used in order to match
 * a given syntax with a given file name: if a match pattern starts with a
 * dot, it is matched as the last past of the filename, for example ".c".
 * Otherwise the pattern is just searched inside the filenme, like "Makefile").
 *
 * The list of keywords to highlight is just a list of words, however if they
 * a trailing '|' character is added at the end, they are highlighted in
 * a different color, so that you can have two different sets of keywords.
 *
 * Finally add a stanza in the HLDB global variable with two two arrays
 * of strings, and a set of flags in order to enable highlighting of
 * comments and numbers.
 *
 * The characters for single and multi line comments must be exactly two
 * and must be provided as well (see the C language example).
 *
 * There is no support to highlight patterns currently. */

#include "def.h"

/* C / C++ */
char *C_HL_extensions[] = {".c",".h",".cpp",".hpp",".cc",NULL};
char *C_HL_keywords[] = {
	/* C Keywords */
	"auto","break","case","continue","default","do","else","enum",
	"extern","for","goto","if","register","return","sizeof","static",
	"struct","switch","typedef","union","volatile","while","NULL",

	/* C++ Keywords */
	"alignas","alignof","and","and_eq","asm","bitand","bitor","class",
	"compl","constexpr","const_cast","deltype","delete","dynamic_cast",
	"explicit","export","false","friend","inline","mutable","namespace",
	"new","noexcept","not","not_eq","nullptr","operator","or","or_eq",
	"private","protected","public","reinterpret_cast","static_assert",
	"static_cast","template","this","thread_local","throw","true","try",
	"typeid","typename","virtual","xor","xor_eq",

	/* C types */
        "int|","long|","double|","float|","char|","unsigned|","signed|",
        "void|","short|","auto|","const|","bool|",NULL
};

/* Python */
char *PYTHON_HL_extensions[] = {".py", ".pyw", ".pyi", ".pyx", NULL};
char *PYTHON_HL_keywords[] = {
    /* Python Keywords */
    "and", "as", "assert", "break", "class", "continue", "def", "del",
    "elif", "else", "except", "exec", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "not", "or", "pass", "print",
    "raise", "return", "try", "while", "with", "yield", "async", "await",
    "nonlocal", "True", "False", "None",

    /* Python Built-ins */
    "abs|", "all|", "any|", "bin|", "bool|", "bytearray|", "bytes|", "callable|",
    "chr|", "classmethod|", "compile|", "complex|", "delattr|", "dict|", "dir|",
    "divmod|", "enumerate|", "eval|", "exec|", "filter|", "float|", "format|",
    "frozenset|", "getattr|", "globals|", "hasattr|", "hash|", "help|", "hex|",
    "id|", "input|", "int|", "isinstance|", "issubclass|", "iter|", "len|",
    "list|", "locals|", "map|", "max|", "memoryview|", "min|", "next|", "object|",
    "oct|", "open|", "ord|", "pow|", "property|", "range|", "repr|", "reversed|",
    "round|", "set|", "setattr|", "slice|", "sorted|", "staticmethod|", "str|",
    "sum|", "super|", "tuple|", "type|", "vars|", "zip|", "self|", "cls|", NULL};

/* Shell */
char *SHELL_HL_extensions[] = {
    ".sh", ".bash", ".zsh", ".ksh", ".csh", ".tcsh",
    ".profile", ".bashrc", ".bash_profile", ".bash_login",
    ".zshrc", ".zshenv", ".zlogin", ".zprofile",
    NULL};

char *SHELL_HL_keywords[] = {
    /* Shell Keywords */
    "if", "then", "else", "elif", "fi", "case", "esac", "for", "while",
    "until", "do", "done", "select", "function", "in", "time", "coproc",

    /* Common commands */
    "alias|", "bg|", "bind|", "break|", "builtin|", "caller|", "cd|",
    "command|", "compgen|", "complete|", "continue|", "declare|",
    "dirs|", "disown|", "echo|", "enable|", "eval|", "exec|", "exit|",
    "export|", "false|", "fc|", "fg|", "getopts|", "hash|", "help|",
    "history|", "jobs|", "kill|", "let|", "local|", "logout|", "mapfile|",
    "popd|", "printf|", "pushd|", "pwd|", "read|", "readarray|",
    "readonly|", "return|", "set|", "shift|", "shopt|", "source|",
    "suspend|", "test|", "times|", "trap|", "true|", "type|", "typeset|",
    "ulimit|", "umask|", "unalias|", "unset|", "wait|",

    /* System utilities */
    "awk|", "cat|", "chmod|", "chown|", "cp|", "curl|", "cut|", "date|",
    "df|", "diff|", "dig|", "du|", "find|", "grep|", "head|", "ln|", "ls|",
    "mkdir|", "mv|", "ping|", "ps|", "rm|", "rsync|", "scp|", "sed|",
    "ssh|", "sudo|", "tail|", "tar|", "top|", "touch|", "tr|", "uniq|",
    "wc|", "wget|", "which|", "xargs|",

    /* Special variables */
    "$BASH|", "$BASHOPTS|", "$BASHPID|", "$BASH_ALIASES|",
    "$BASH_ARGC|", "$BASH_ARGV|", "$BASH_CMDS|", "$BASH_COMMAND|",
    "$BASH_ENV|", "$BASH_LINENO|", "$BASH_SOURCE|", "$BASH_SUBSHELL|",
    "$BASH_VERSION|", "$DIRSTACK|", "$EUID|", "$FUNCNAME|",
    "$GROUPS|", "$HOME|", "$HOSTNAME|", "$HOSTTYPE|", "$IFS|",
    "$LINENO|", "$MACHTYPE|", "$OLDPWD|", "$OPTARG|", "$OPTIND|",
    "$OSTYPE|", "$PATH|", "$PIPESTATUS|", "$PPID|", "$PS1|",
    "$PS2|", "$PS3|", "$PS4|", "$PWD|", "$RANDOM|", "$REPLY|",
    "$SECONDS|", "$SHELL|", "$SHELLOPTS|", "$SHLVL|", "$UID|",
    NULL};

/* JavaScript */
char *JS_HL_extensions[] = {".js", ".jsx", ".mjs", ".cjs", NULL};
char *JS_HL_keywords[] = {
    /* JavaScript Keywords */
    "break", "case", "catch", "class", "const", "continue", "debugger", "default",
    "delete", "do", "else", "export", "extends", "finally", "for", "function",
    "if", "import", "in", "instanceof", "let", "new", "return", "super", "switch",
    "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",
    "async", "await", "of", "true", "false", "null", "undefined",

    /* JavaScript Built-ins */
    "Array|", "Object|", "String|", "Number|", "Boolean|", "Date|", "Math|",
    "RegExp|", "Error|", "JSON|", "console|", "window|", "document|", "setTimeout|",
    "setInterval|", "clearTimeout|", "clearInterval|", "parseInt|", "parseFloat|",
    "isNaN|", "isFinite|", "encodeURI|", "decodeURI|", "Promise|", "Map|", "Set|",
    "WeakMap|", "WeakSet|", "Symbol|", "Proxy|", "Reflect|", "Generator|", NULL};

/* Rust */
char *RUST_HL_extensions[] = {".rs", ".rlib", NULL};
char *RUST_HL_keywords[] = {
    /* Rust Keywords */
    "as", "async", "await", "break", "const", "continue", "crate", "dyn", "else",
    "enum", "extern", "false", "fn", "for", "if", "impl", "in", "let", "loop",
    "match", "mod", "move", "mut", "pub", "ref", "return", "self", "Self", "static",
    "struct", "super", "trait", "true", "type", "unsafe", "use", "where", "while",
    "abstract", "become", "box", "do", "final", "macro", "override", "priv",
    "typeof", "unsized", "virtual", "yield", "try", "union", "catch", "default",

    /* Rust Types */
    "i8|", "i16|", "i32|", "i64|", "i128|", "isize|", "u8|", "u16|", "u32|", "u64|",
    "u128|", "usize|", "f32|", "f64|", "bool|", "char|", "str|", "String|", "Vec|",
    "HashMap|", "HashSet|", "BTreeMap|", "BTreeSet|", "Option|", "Result|", "Box|",
    "Rc|", "Arc|", "RefCell|", "Cell|", "Mutex|", "RwLock|", "thread|", "Clone|",
    "Copy|", "Send|", "Sync|", "Drop|", "Display|", "Debug|", "Default|", "PartialEq|",
    "Eq|", "PartialOrd|", "Ord|", "Hash|", "Iterator|", "IntoIterator|", NULL};

/* Java */
char *JAVA_HL_extensions[] = {".java", ".class", NULL};
char *JAVA_HL_keywords[] = {
    /* Java Keywords */
    "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char",
    "class", "const", "continue", "default", "do", "double", "else", "enum",
    "extends", "final", "finally", "float", "for", "goto", "if", "implements",
    "import", "instanceof", "int", "interface", "long", "native", "new", "package",
    "private", "protected", "public", "return", "short", "static", "strictfp",
    "super", "switch", "synchronized", "this", "throw", "throws", "transient",
    "try", "void", "volatile", "while", "true", "false", "null",

    /* Java Types and Common Classes */
    "String|", "Object|", "Class|", "System|", "Thread|", "Runnable|",
    "Exception|", "RuntimeException|", "ArrayList|", "HashMap|", "List|",
    "Map|", "Set|", "Collection|", "Iterator|", "Comparable|", "Serializable|", NULL};

/* TypeScript */
char *TS_HL_extensions[] = {".ts", ".tsx", ".d.ts", NULL};
char *TS_HL_keywords[] = {
    /* TypeScript Keywords (includes JavaScript) */
    "break", "case", "catch", "class", "const", "continue", "debugger", "default",
    "delete", "do", "else", "export", "extends", "finally", "for", "function",
    "if", "import", "in", "instanceof", "let", "new", "return", "super", "switch",
    "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",
    "async", "await", "of", "true", "false", "null", "undefined",

    /* TypeScript Specific */
    "interface", "type", "enum", "namespace", "module", "declare", "abstract",
    "implements", "private", "protected", "public", "readonly", "static",
    "get", "set", "as", "keyof", "infer", "is", "asserts",

    /* TypeScript Types */
    "string|", "number|", "boolean|", "object|", "any|", "unknown|", "never|",
    "void|", "bigint|", "symbol|", "Array|", "Promise|", "Record|", "Partial|",
    "Required|", "Pick|", "Omit|", "Exclude|", "Extract|", "NonNullable|", NULL};

/* C# */
char *CSHARP_HL_extensions[] = {".cs", ".csx", NULL};
char *CSHARP_HL_keywords[] = {
    /* C# Keywords */
    "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char",
    "checked", "class", "const", "continue", "decimal", "default", "delegate",
    "do", "double", "else", "enum", "event", "explicit", "extern", "false",
    "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit",
    "in", "int", "interface", "internal", "is", "lock", "long", "namespace",
    "new", "null", "object", "operator", "out", "override", "params", "private",
    "protected", "public", "readonly", "ref", "return", "sbyte", "sealed",
    "short", "sizeof", "stackalloc", "static", "string", "struct", "switch",
    "this", "throw", "true", "try", "typeof", "uint", "ulong", "unchecked",
    "unsafe", "ushort", "using", "virtual", "void", "volatile", "while",
    "async", "await", "var", "dynamic", "yield", "where", "when", "nameof",

    /* C# Types */
    "String|", "Object|", "Int32|", "Boolean|", "Double|", "DateTime|", "List|",
    "Dictionary|", "Array|", "IEnumerable|", "ICollection|", "IList|", "Task|",
    "Exception|", "ArgumentException|", "NullReferenceException|", NULL};

/* PHP */
char *PHP_HL_extensions[] = {".php", ".phtml", ".php3", ".php4", ".php5", ".phps", NULL};
char *PHP_HL_keywords[] = {
    /* PHP Keywords */
    "abstract", "and", "array", "as", "break", "callable", "case", "catch",
    "class", "clone", "const", "continue", "declare", "default", "die", "do",
    "echo", "else", "elseif", "empty", "enddeclare", "endfor", "endforeach",
    "endif", "endswitch", "endwhile", "eval", "exit", "extends", "final",
    "finally", "for", "foreach", "function", "global", "goto", "if", "implements",
    "include", "include_once", "instanceof", "insteadof", "interface", "isset",
    "list", "namespace", "new", "or", "print", "private", "protected", "public",
    "require", "require_once", "return", "static", "switch", "throw", "trait",
    "try", "unset", "use", "var", "while", "xor", "yield", "true", "false", "null",

    /* PHP Built-ins */
    "$_GET|", "$_POST|", "$_SESSION|", "$_COOKIE|", "$_SERVER|", "$_FILES|",
    "$_ENV|", "$_REQUEST|", "$GLOBALS|", "strlen|", "substr|", "strpos|",
    "explode|", "implode|", "array_merge|", "array_push|", "array_pop|",
    "count|", "sizeof|", "is_array|", "is_string|", "is_numeric|", "empty|",
    "isset|", "unset|", "die|", "exit|", "echo|", "print|", "var_dump|", NULL};

/* Ruby */
char *RUBY_HL_extensions[] = {".rb", ".rbw", ".rake", ".gemspec", NULL};
char *RUBY_HL_keywords[] = {
    /* Ruby Keywords */
    "alias", "and", "begin", "break", "case", "class", "def", "defined", "do",
    "else", "elsif", "end", "ensure", "false", "for", "if", "in", "module",
    "next", "nil", "not", "or", "redo", "rescue", "retry", "return", "self",
    "super", "then", "true", "undef", "unless", "until", "when", "while", "yield",
    "require", "include", "extend", "attr_reader", "attr_writer", "attr_accessor",

    /* Ruby Built-ins */
    "puts|", "print|", "p|", "gets|", "chomp|", "strip|", "length|", "size|",
    "empty|", "nil|", "class|", "new|", "initialize|", "to_s|", "to_i|", "to_f|",
    "to_a|", "each|", "map|", "select|", "reject|", "find|", "inject|", "reduce|",
    "Array|", "Hash|", "String|", "Integer|", "Float|", "Symbol|", "Proc|",
    "Lambda|", "Method|", "Class|", "Module|", "Object|", "Kernel|", NULL};

/* Swift */
char *SWIFT_HL_extensions[] = {".swift", NULL};
char *SWIFT_HL_keywords[] = {
    /* Swift Keywords */
    "associatedtype", "class", "deinit", "enum", "extension", "fileprivate", "func",
    "import", "init", "inout", "internal", "let", "open", "operator", "private",
    "protocol", "public", "static", "struct", "subscript", "typealias", "var",
    "break", "case", "continue", "default", "defer", "do", "else", "fallthrough",
    "for", "guard", "if", "in", "repeat", "return", "switch", "where", "while",
    "as", "catch", "false", "is", "nil", "rethrows", "super", "self", "Self",
    "throw", "throws", "true", "try", "async", "await", "some", "any",

    /* Swift Types */
    "Int|", "Double|", "Float|", "Bool|", "String|", "Character|", "Array|",
    "Dictionary|", "Set|", "Optional|", "Result|", "Error|", "AnyObject|",
    "AnyClass|", "Protocol|", "Codable|", "Hashable|", "Equatable|",
    "Comparable|", "Collection|", "Sequence|", NULL};

/* SQL */
char *SQL_HL_extensions[] = {".sql", ".ddl", ".dml", NULL};
char *SQL_HL_keywords[] = {
    /* SQL Keywords */
    "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "CREATE", "DROP",
    "ALTER", "TABLE", "INDEX", "VIEW", "DATABASE", "SCHEMA", "COLUMN", "PRIMARY",
    "FOREIGN", "KEY", "REFERENCES", "CONSTRAINT", "UNIQUE", "NOT", "NULL", "DEFAULT",
    "AUTO_INCREMENT", "IDENTITY", "SERIAL", "BOOLEAN", "TINYINT", "SMALLINT",
    "MEDIUMINT", "INT", "INTEGER", "BIGINT", "DECIMAL", "NUMERIC", "FLOAT", "DOUBLE",
    "REAL", "BIT", "DATE", "TIME", "DATETIME", "TIMESTAMP", "YEAR", "CHAR", "VARCHAR",
    "BINARY", "VARBINARY", "TINYBLOB", "BLOB", "MEDIUMBLOB", "LONGBLOB", "TINYTEXT",
    "TEXT", "MEDIUMTEXT", "LONGTEXT", "ENUM", "SET", "JSON", "GEOMETRY", "POINT",
    "LINESTRING", "POLYGON", "MULTIPOINT", "MULTILINESTRING", "MULTIPOLYGON",
    "GEOMETRYCOLLECTION", "AND", "OR", "IN", "BETWEEN", "LIKE", "IS", "EXISTS",
    "ANY", "ALL", "SOME", "UNION", "INTERSECT", "EXCEPT", "INNER", "LEFT", "RIGHT",
    "FULL", "OUTER", "JOIN", "ON", "USING", "GROUP", "BY", "HAVING", "ORDER", "ASC",
    "DESC", "LIMIT", "OFFSET", "DISTINCT", "AS", "CASE", "WHEN", "THEN", "ELSE", "END",
    "IF", "IFNULL", "ISNULL", "COALESCE", "NULLIF", "CAST", "CONVERT", "SUBSTRING",
    "LENGTH", "UPPER", "LOWER", "TRIM", "LTRIM", "RTRIM", "REPLACE", "CONCAT",
    "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "NOW", "COUNT", "SUM",
    "AVG", "MIN", "MAX", "STDDEV", "VARIANCE", "BEGIN", "COMMIT", "ROLLBACK",
    "TRANSACTION", "SAVEPOINT", "GRANT", "REVOKE", "LOCK", "UNLOCK",

    /* SQL Functions and Operators */
    "TRUE|", "FALSE|", "UNKNOWN|", NULL};

/* Dart */
char *DART_HL_extensions[] = {".dart", NULL};
char *DART_HL_keywords[] = {
    /* Dart Keywords */
    "abstract", "as", "assert", "async", "await", "break", "case", "catch", "class",
    "const", "continue", "covariant", "default", "deferred", "do", "dynamic", "else",
    "enum", "export", "extends", "extension", "external", "factory", "false", "final",
    "finally", "for", "Function", "get", "hide", "if", "implements", "import", "in",
    "interface", "is", "late", "library", "mixin", "new", "null", "on", "operator",
    "part", "required", "rethrow", "return", "set", "show", "static", "super", "switch",
    "sync", "this", "throw", "true", "try", "typedef", "var", "void", "while", "with",
    "yield",

    /* Dart Types */
    "int|", "double|", "num|", "String|", "bool|", "List|", "Map|", "Set|", "Object|",
    "dynamic|", "var|", "void|", "Future|", "Stream|", "Iterable|", "Iterator|",
    "Comparable|", "Duration|", "DateTime|", "Uri|", "RegExp|", "StringBuffer|",
    "Symbol|", "Type|", "Function|", "Null|", NULL};

/* HTML */
char *HTML_HL_extensions[] = {".html", ".htm", ".xhtml", NULL};
char *HTML_HL_keywords[] = {
    /* Opening tags */
    "<a>", "<abbr>", "<address>", "<article>", "<aside>", "<audio>",
    "<b>", "<bdi>", "<bdo>", "<blockquote>", "<body>", "<br>", "<button>",
    "<canvas>", "<caption>", "<cite>", "<code>", "<colgroup>",
    "<datalist>", "<dd>", "<del>", "<details>", "<dfn>", "<dialog>",
    "<div>", "<dl>", "<dt>", "<em>", "<embed>",
    "<fieldset>", "<figcaption>", "<figure>", "<footer>", "<form>",
    "<h1>", "<h2>", "<h3>", "<h4>", "<h5>", "<h6>", "<head>", "<header>", "<hr>", "<html>",
    "<i>", "<iframe>", "<img>", "<input>", "<ins>",
    "<kbd>", "<label>", "<legend>", "<li>", "<link>",
    "<main>", "<map>", "<mark>", "<meta>", "<meter>",
    "<nav>", "<noscript>",
    "<object>", "<ol>", "<optgroup>", "<option>", "<output>",
    "<p>", "<picture>", "<pre>", "<progress>",
    "<q>",
    "<s>", "<samp>", "<script>", "<section>", "<select>", "<small>", "<source>",
    "<span>", "<strong>", "<style>", "<sub>", "<summary>", "<sup>", "<svg>",
    "<table>", "<tbody>", "<td>", "<template>", "<textarea>", "<tfoot>", "<th>", "<thead>",
    "<time>", "<title>", "<tr>", "<track>",
    "<u>", "<ul>",
    "<var>", "<video>",

    /* Closing tags */
    "</a>", "</abbr>", "</address>", "</article>", "</aside>", "</audio>",
    "</b>", "</bdi>", "</bdo>", "</blockquote>", "</body>", "</button>",
    "</canvas>", "</caption>", "</cite>", "</code>", "</colgroup>",
    "</datalist>", "</dd>", "</del>", "</details>", "</dfn>", "</dialog>",
    "</div>", "</dl>", "</dt>", "</em>",
    "</fieldset>", "</figcaption>", "</figure>", "</footer>", "</form>",
    "</h1>", "</h2>", "</h3>", "</h4>", "</h5>", "</h6>", "</head>", "</header>", "</html>",
    "</i>", "</iframe>", "</ins>",
    "</kbd>", "</label>", "</legend>", "</li>",
    "</main>", "</map>", "</mark>", "</meter>",
    "</nav>", "</noscript>",
    "</object>", "</ol>", "</optgroup>", "</option>", "</output>",
    "</p>", "</picture>", "</pre>", "</progress>",
    "</q>",
    "</s>", "</samp>", "</script>", "</section>", "</select>", "</small>",
    "</span>", "</strong>", "</style>", "</sub>", "</summary>", "</sup>", "</svg>",
    "</table>", "</tbody>", "</td>", "</template>", "</textarea>", "</tfoot>", "</th>", "</thead>",
    "</time>", "</title>", "</tr>",
    "</u>", "</ul>",
    "</var>", "</video>",

    /* Common attributes */
    "class=|", "id=|", "style=|", "src=|", "href=|", "alt=|", "title=|",
    "width=|", "height=|", "type=|", "name=|", "value=|", "placeholder=|",
    NULL};

/* React/JSX - extends JavaScript with React-specific features */
char *REACT_HL_extensions[] = {".jsx", NULL};
char *REACT_HL_keywords[] = {
    /* All JavaScript keywords first */
    "async", "await", "break", "case", "catch", "class", "const", "continue",
    "debugger", "default", "delete", "do", "else", "export", "extends",
    "finally", "for", "from", "function", "if", "import", "in",
    "instanceof", "let", "new", "return", "static", "super", "switch",
    "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",

    /* React Hooks */
    "useState|", "useEffect|", "useContext|", "useReducer|", "useCallback|",
    "useMemo|", "useRef|", "useImperativeHandle|", "useLayoutEffect|",
    "useDebugValue|", "useDeferredValue|", "useTransition|", "useId|",
    "useSyncExternalStore|", "useInsertionEffect|",

    /* React API */
    "React|", "Component|", "PureComponent|", "Fragment|", "StrictMode|",
    "Suspense|", "createElement|", "createContext|", "forwardRef|", "lazy|",
    "memo|", "createRef|", "isValidElement|", "Children|", "cloneElement|",

    /* Common JSX/React patterns */
    "props|", "state|", "key|", "ref|", "defaultProps|", "propTypes|",
    "className|", "onClick|", "onChange|", "onSubmit|", "useState|",

    /* Common values */
    "true|", "false|", "null|", "undefined|", "NaN|", "Infinity|",

    /* Built-in objects */
    "Array|", "Object|", "String|", "Number|", "Boolean|", "Date|", "Math|",
    "JSON|", "Promise|", "Map|", "Set|", "WeakMap|", "WeakSet|",
    "Symbol|", "BigInt|", "RegExp|", "Error|", "console|",
    NULL};

/* Vue.js - single file component syntax */
char *VUE_HL_extensions[] = {".vue", NULL};
char *VUE_HL_keywords[] = {
    /* JavaScript keywords */
    "async", "await", "break", "case", "catch", "class", "const", "continue",
    "debugger", "default", "delete", "do", "else", "export", "extends",
    "finally", "for", "from", "function", "if", "import", "in",
    "instanceof", "let", "new", "return", "static", "super", "switch",
    "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",

    /* Vue Composition API */
    "ref|", "reactive|", "computed|", "watch|", "watchEffect|", "onMounted|",
    "onUpdated|", "onUnmounted|", "onBeforeMount|", "onBeforeUpdate|",
    "onBeforeUnmount|", "onActivated|", "onDeactivated|", "onErrorCaptured|",
    "provide|", "inject|", "defineProps|", "defineEmits|", "defineExpose|",
    "useSlots|", "useAttrs|", "toRef|", "toRefs|", "isRef|", "unref|",
    "shallowRef|", "triggerRef|", "customRef|", "shallowReactive|",
    "readonly|", "shallowReadonly|", "toRaw|", "markRaw|",

    /* Vue Options API */
    "data|", "props|", "methods|", "computed|", "watch|", "emits|",
    "components|", "directives|", "mixins|", "extends|", "setup|",
    "beforeCreate|", "created|", "beforeMount|", "mounted|",
    "beforeUpdate|", "updated|", "beforeUnmount|", "unmounted|",

    /* Vue Directives */
    "v-if|", "v-else|", "v-else-if|", "v-for|", "v-show|", "v-bind|",
    "v-on|", "v-model|", "v-slot|", "v-pre|", "v-once|", "v-memo|",
    "v-cloak|", "v-html|", "v-text|",

    /* Vue Special Attributes */
    "key|", "ref|", "is|",

    /* Common values */
    "true|", "false|", "null|", "undefined|", "NaN|", "Infinity|",

    /* Built-in objects */
    "Array|", "Object|", "String|", "Number|", "Boolean|", "Date|", "Math|",
    "JSON|", "Promise|", "Map|", "Set|", "console|",

    /* Vue template tags */
    "<template>", "</template>", "<script>", "</script>", "<style>", "</style>",
    NULL};

/* Angular - TypeScript with Angular decorators and directives */
char *ANGULAR_HL_extensions[] = {".component.ts", ".service.ts", ".module.ts",
                                  ".directive.ts", ".pipe.ts", ".guard.ts", NULL};
char *ANGULAR_HL_keywords[] = {
    /* TypeScript/JavaScript keywords */
    "abstract", "async", "await", "break", "case", "catch", "class", "const",
    "continue", "debugger", "default", "delete", "do", "else", "enum",
    "export", "extends", "finally", "for", "from", "function", "if",
    "implements", "import", "in", "instanceof", "interface", "let", "new",
    "private", "protected", "public", "readonly", "return", "static",
    "super", "switch", "this", "throw", "try", "type", "typeof", "var",
    "void", "while", "with", "yield",

    /* Angular Decorators */
    "@Component|", "@NgModule|", "@Injectable|", "@Directive|", "@Pipe|",
    "@Input|", "@Output|", "@ViewChild|", "@ViewChildren|", "@ContentChild|",
    "@ContentChildren|", "@HostBinding|", "@HostListener|",

    /* Angular Core */
    "OnInit|", "OnDestroy|", "OnChanges|", "DoCheck|", "AfterContentInit|",
    "AfterContentChecked|", "AfterViewInit|", "AfterViewChecked|",
    "ChangeDetectorRef|", "ElementRef|", "Renderer2|", "ViewContainerRef|",
    "TemplateRef|", "EventEmitter|", "Injector|", "ComponentFactoryResolver|",

    /* Angular Common */
    "ngFor|", "ngIf|", "ngSwitch|", "ngClass|", "ngStyle|", "ngModel|",
    "FormControl|", "FormGroup|", "FormBuilder|", "Validators|",
    "HttpClient|", "HttpHeaders|", "Observable|", "Subject|", "BehaviorSubject|",
    "Router|", "ActivatedRoute|", "RouterModule|", "Routes|",

    /* Common values */
    "true|", "false|", "null|", "undefined|", "NaN|", "Infinity|",

    /* Built-in types */
    "string|", "number|", "boolean|", "any|", "void|", "never|", "unknown|",
    "Array|", "Object|", "Promise|", "Map|", "Set|",
    NULL};

/* Svelte - single file component with reactive syntax */
char *SVELTE_HL_extensions[] = {".svelte", NULL};
char *SVELTE_HL_keywords[] = {
    /* JavaScript keywords */
    "async", "await", "break", "case", "catch", "class", "const", "continue",
    "debugger", "default", "delete", "do", "else", "export", "extends",
    "finally", "for", "from", "function", "if", "import", "in",
    "instanceof", "let", "new", "return", "static", "super", "switch",
    "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",

    /* Svelte Lifecycle */
    "onMount|", "onDestroy|", "beforeUpdate|", "afterUpdate|", "tick|",
    "setContext|", "getContext|", "hasContext|", "getAllContexts|",

    /* Svelte Stores */
    "writable|", "readable|", "derived|", "get|",

    /* Svelte Motion */
    "tweened|", "spring|",

    /* Svelte Transitions */
    "fade|", "blur|", "fly|", "slide|", "scale|", "draw|", "crossfade|",

    /* Svelte Actions */
    "use|",

    /* Svelte Bindings */
    "bind|", "on|", "class|",

    /* Svelte Special Elements */
    "svelte:component|", "svelte:window|", "svelte:body|", "svelte:head|",
    "svelte:options|", "svelte:fragment|", "svelte:self|",

    /* Svelte Blocks */
    "{#if", "{:else", "{:else if", "{/if}",
    "{#each", "{/each}",
    "{#await", "{:then", "{:catch", "{/await}",
    "{#key", "{/key}",

    /* Common values */
    "true|", "false|", "null|", "undefined|", "NaN|", "Infinity|",

    /* Built-in objects */
    "Array|", "Object|", "String|", "Number|", "Boolean|", "Date|", "Math|",
    "JSON|", "Promise|", "Map|", "Set|", "console|",

    /* Svelte template tags */
    "<script>", "</script>", "<style>", "</style>",
    NULL};

/* Here we define an array of syntax highlights by extensions, keywords,
 * comments delimiters and flags. */
struct editorSyntax HLDB[] = {
    {
        /* C / C++ */
        C_HL_extensions,
        C_HL_keywords,
        "//","/*","*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Python */
        PYTHON_HL_extensions,
        PYTHON_HL_keywords,
        "#", "", "",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Shell */
        SHELL_HL_extensions,
        SHELL_HL_keywords,
        "#", "", "",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* JavaScript */
        JS_HL_extensions,
        JS_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Rust */
        RUST_HL_extensions,
        RUST_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Java */
        JAVA_HL_extensions,
        JAVA_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* TypeScript */
        TS_HL_extensions,
        TS_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* C# */
        CSHARP_HL_extensions,
        CSHARP_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* PHP */
        PHP_HL_extensions,
        PHP_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Ruby */
        RUBY_HL_extensions,
        RUBY_HL_keywords,
        "#", "", "",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Swift */
        SWIFT_HL_extensions,
        SWIFT_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* SQL */
        SQL_HL_extensions,
        SQL_HL_keywords,
        "--", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Dart */
        DART_HL_extensions,
        DART_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* HTML */
        HTML_HL_extensions,
        HTML_HL_keywords,
        "<!--", "", "-->",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* React/JSX */
        REACT_HL_extensions,
        REACT_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Vue.js */
        VUE_HL_extensions,
        VUE_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Angular */
        ANGULAR_HL_extensions,
        ANGULAR_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    },
    {
        /* Svelte */
        SVELTE_HL_extensions,
        SVELTE_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    }
};

#define HLDB_ENTRIES (sizeof(HLDB)/sizeof(HLDB[0]))

/* ====================== Syntax highlight color scheme  ==================== */

int is_separator(int c) {
    return c == '\0' || isspace(c) || strchr(",.()+-/*=~%[];",c) != NULL;
}

/* Return true if the specified row last char is part of a multi line comment
 * that starts at this row or at one before, and does not end at the end
 * of the row but spawns to the next row. */
int editorRowHasOpenComment(erow *row) {
    if (row->hl && row->rsize && row->hl[row->rsize-1] == HL_MLCOMMENT &&
        (row->rsize < 2 || (row->render[row->rsize-2] != '*' ||
                            row->render[row->rsize-1] != '/'))) return 1;
    return 0;
}

/* Set every byte of row->hl (that corresponds to every character in the line)
 * to the right syntax highlight type (HL_* defines). */
void editorUpdateSyntax(erow *row) {
    row->hl = realloc(row->hl,row->rsize);
    memset(row->hl,HL_NORMAL,row->rsize);

    if (E.syntax == NULL) return; /* No syntax, everything is HL_NORMAL. */

    int i, prev_sep, in_string, in_comment;
    char *p;
    char **keywords = E.syntax->keywords;
    char *scs = E.syntax->singleline_comment_start;
    char *mcs = E.syntax->multiline_comment_start;
    char *mce = E.syntax->multiline_comment_end;

    /* Point to the first non-space char. */
    p = row->render;
    i = 0; /* Current char offset */
    while(*p && isspace(*p)) {
        p++;
        i++;
    }
    prev_sep = 1; /* Tell the parser if 'i' points to start of word. */
    in_string = 0; /* Are we inside "" or '' ? */
    in_comment = 0; /* Are we inside multi-line comment? */

    /* If the previous line has an open comment, this line starts
     * with an open comment state. */
    if (row->idx > 0 && editorRowHasOpenComment(&E.row[row->idx-1]))
        in_comment = 1;

    while(*p) {
        /* Handle // comments. */
        if (prev_sep && *p == scs[0] && *(p+1) == scs[1]) {
            /* From here to end is a comment */
            memset(row->hl+i,HL_COMMENT,row->size-i);
            return;
        }

        /* Handle multi line comments. */
        if (in_comment) {
            row->hl[i] = HL_MLCOMMENT;
            if (*p == mce[0] && *(p+1) == mce[1]) {
                row->hl[i+1] = HL_MLCOMMENT;
                p += 2; i += 2;
                in_comment = 0;
                prev_sep = 1;
                continue;
            } else {
                prev_sep = 0;
                p++; i++;
                continue;
            }
        } else if (*p == mcs[0] && *(p+1) == mcs[1]) {
            row->hl[i] = HL_MLCOMMENT;
            row->hl[i+1] = HL_MLCOMMENT;
            p += 2; i += 2;
            in_comment = 1;
            prev_sep = 0;
            continue;
        }

        /* Handle "" and '' */
        if (in_string) {
            row->hl[i] = HL_STRING;
            if (*p == '\\') {
                row->hl[i+1] = HL_STRING;
                p += 2; i += 2;
                prev_sep = 0;
                continue;
            }
            if (*p == in_string) in_string = 0;
            p++; i++;
            continue;
        } else {
            if (*p == '"' || *p == '\'') {
                in_string = *p;
                row->hl[i] = HL_STRING;
                p++; i++;
                prev_sep = 0;
                continue;
            }
        }

        /* Handle non printable chars. */
        if (!isprint(*p)) {
            row->hl[i] = HL_NONPRINT;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        /* Handle numbers */
        if ((isdigit(*p) && (prev_sep || row->hl[i-1] == HL_NUMBER)) ||
            (*p == '.' && i >0 && row->hl[i-1] == HL_NUMBER)) {
            row->hl[i] = HL_NUMBER;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        /* Handle keywords and lib calls */
        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen-1] == '|';
                if (kw2) klen--;

                if (!memcmp(p,keywords[j],klen) &&
                    is_separator(*(p+klen)))
                {
                    /* Keyword */
                    memset(row->hl+i,kw2 ? HL_KEYWORD2 : HL_KEYWORD1,klen);
                    p += klen;
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue; /* We had a keyword match */
            }
        }

        /* Not special chars */
        prev_sep = is_separator(*p);
        p++; i++;
    }

    /* Propagate syntax change to the next row if the open commen
     * state changed. This may recursively affect all the following rows
     * in the file. */
    int oc = editorRowHasOpenComment(row);
    if (row->hl_oc != oc && row->idx+1 < E.numrows)
        editorUpdateSyntax(&E.row[row->idx+1]);
    row->hl_oc = oc;
}

/* Maps syntax highlight token types to terminal colors. */
int editorSyntaxToColor(int hl) {
    switch(hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return 36;     /* cyan */
    case HL_KEYWORD1: return 33;    /* yellow */
    case HL_KEYWORD2: return 32;    /* green */
    case HL_STRING: return 35;      /* magenta */
    case HL_NUMBER: return 31;      /* red */
    case HL_MATCH: return 34;      /* blu */
    default: return 37;             /* white */
    }
}

/* Select the syntax highlight scheme depending on the filename,
 * setting it in the global state E.syntax. */
void editorSelectSyntaxHighlight(char *filename) {
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = HLDB+j;
        unsigned int i = 0;
        while(s->filematch[i]) {
            char *p;
            int patlen = strlen(s->filematch[i]);
            if ((p = strstr(filename,s->filematch[i])) != NULL) {
                if (s->filematch[i][0] != '.' || p[patlen] == '\0') {
                    E.syntax = s;
                    return;
                }
            }
            i++;
        }
    }
}
