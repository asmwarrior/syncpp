/*
 * Copyright 2014 Anton Karmanov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Fibonacci Numbers - a sample dynamic server page.

;;;

const MIN_VALUE = 1;
const MAX_VALUE = 1000;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Big integer arithmetic
//
// Big numbers are stored as arrays of decimal digits, 0-9, the least significant digit at 0-th
// position.

// Returns the effective length of a big integer (excluding leading zeros).
function bigint_len(v) {
    var len = v.length;
    while (len > 0 && v[len - 1] == 0) --len;
    return len;
}

// Returns an integral decimal string representation of the specified big integer.
function bigint_to_str(v) {
    const len = bigint_len(v);
    if (len == 0) return "0";

    const digits = "0123456789";
    
    const buf = new sys.StringBuffer();
    for (var i = len - 1; i >= 0; --i) buf.append_char(digits[v[i]]);

    return buf.to_string();
}

// Returns a short floating-point string representation of the specified big integer
// (e.g. 1.5e+53).
function bigint_to_float_str(v) {
    const len = bigint_len(v);
    if (len == 0) return "0";
    if (len == 1) return v[0] + ".0";
    
    return v[len - 1] + "." + v[len - 2] + "e+" + (len - 1);
}

// Adds two big integers.
function bigint_add(a, b) {
    var a_len = bigint_len(a);
    if (a_len == 0) return b;
    var b_len = bigint_len(b);
    if (b_len == 0) return a;

    if (a_len < b_len) {
        var c = a;
        a = b;
        b = c;
        var c_len = a_len;
        a_len = b_len;
        b_len = c_len;
    }

    var r_len = a_len + 1;
    var r = new[r_len];

    var d = 0;
    for (var i = 0; i < b_len; ++i) {
        var s = d + a[i] + b[i];
        d = 0;
        if (s >= 10) {
            d = 1;
            s -= 10;
        }
        r[i] = s;
    }

    for (var i = b_len; i < a_len; ++i) {
        var s = d + a[i];
        d = 0;
        if (s >= 10) {
            d = 1;
            s -= 10;
        }
        r[i] = s;
    }

    r[a_len] = d;

    return r;
}

// Calculates Fibonacci Numbers for the specified range of an argument.
// Calls the given function for each number.
function generate_fibonacci(from, to, fn) {
    var a = [];
    var b = [1];

    for (var i = 1; i < from; ++i) {
        var c = bigint_add(a, b);
        a = b;
        b = c;
    }

    for (var i = from; i <= to; ++i) {
        var c = bigint_add(a, b);
        fn(b);
        a = b;
        b = c;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// HTML Generation.

const doc = new HTML_Document(http.resp);

// Converts a string to an input value from range MIN_VALUE ... MAX_VALUE.
function str_to_value(str) {
    if (str == null) return null;
    try {
        var v = sys.str_to_int(str);
        if (v < MIN_VALUE) v = MIN_VALUE;
        if (v > MAX_VALUE) v = MAX_VALUE;
        return v;
    } catch (e) {
        return null;
    }
}

const from_str = http.req.get_parameter("from");
const to_str = http.req.get_parameter("to");
const from_value = str_to_value(from_str);
const to_value = str_to_value(to_str);
const input_valid = from_value != null && to_value != null && from_value <= to_value;

// Generates the parameters HTML form.
function data_form() {
    const from_to_specified = from_str != null && to_str != null;
    const input_invalid = from_to_specified && !input_valid;
    
    var def_from = "1";
    var def_to = "50";
    if (input_valid) {
        def_from = "" + from_value;
        def_to = "" + to_value;
    } else if (from_to_specified) {
        def_from = from_str;
        def_to = to_str;
    }

    doc.data(
        ["form",
            ["div class='main_header'", "FIBONACCI NUMBERS"],
            ["table",
                ["tr",
                    ["td align='left' style='padding-top:1em'",
                        ["table",
                            ["tr",
                                ["th class='form_th'", "From:"],
                                ["td", ["input type='text' name='from' class='form_input' autocomplete='off' value='" + html_encode(def_from) + "'"]]
                            ],
                            ["tr",
                                ["th class='form_th'", "To:"],
                                ["td", ["input type='text' name='to' class='form_input' autocomplete='off' value='" + html_encode(def_to) + "'"]]
                            ],
                            ["tr", ["td"], ["td class='form_hint'", "( <b>" + MIN_VALUE + "</b> ... <b>" + MAX_VALUE + "</b> )"]],
                            ["tr",
                                ["td colspan='2' align='center' style='padding-top:0.5em;'",
                                    ["input type='submit' class='form_submit' value='Calculate'"]
                                ]
                            ]
                        ]
                    ]
                ],
                input_invalid ? ["tr", ["td colspan='2' class='form_error'", "Invalid input value(s)!"]] : []
            ]
        ]);
}

// Generates the Fibonacci Numbers HTML table.
function data_numbers() {
    if (!input_valid) return [];

    const list = new sys.ArrayList();
    
    doc.tag("table class='nums_table'");
    
    doc.data(
        ["tr",
            ["th class='nums_th'", "n"],
            ["th class='nums_th'", "F<sub>n</sub>&nbsp;(float)"],
            ["th class='nums_th'", "F<sub>n</sub>"]
        ]);

    var index = from_value;
    generate_fibonacci(from_value, to_value, (v){
        doc.data(
            ["tr",
                ["th class='nums_td'", "" + (index++)],
                ["td class='nums_td'", "" + bigint_to_float_str(v)],
                ["td class='nums_td'", "" + bigint_to_str(v)]
            ]);
    });
        
    doc.end();
}

function load_file(file_name) { return new sys.File(server.root_dir, file_name).read_text(); }

const data =
    ["html",
        ["head",
            ["style", load_file("common.css")],
            ["style", load_file("fibonacci.css")],
            ["title", "Fibonacci Numbers"]
        ],
        ["body", data_form, ["p"], data_numbers]
    ];

doc.data(data);
doc.close();
