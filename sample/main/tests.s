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

//Unit tests for some features of the Script Language.

function assert(b) {
	if (!b) throw "* Test Failed *";
}

function assertEq(expected, was) {
	function value2str(v) {
		return v == null ? "null" : "<" + v + ">";
	}
	if (expected != was) throw "* Test Failed : expected " + value2str(expected) + ", was " + value2str(was) + " *";
}

var tests = [
	{
		var array = [];
		assertEq(0, array.length);
		array = [ 1, 2, 3, 4, 5 ];
		assertEq(5, array.length);
	},
	{
		var str = "";
		assertEq(0, str.length());
		str = "Hello";
		assertEq(5, str.length());
	},
	{
		var str = "Hello";
		assertEq('H', str.char_at(0));
		assertEq('e', str.char_at(1));
		assertEq('l', str.char_at(2));
		assertEq('l', str.char_at(3));
		assertEq('o', str.char_at(4));
	},
	{
		assertEq("", "" + "");
		assertEq("AB", "AB" + "");
		assertEq("AB", "" + "AB");
		assertEq("ABCD", "AB" + "CD");
		assertEq("ab12", "ab" + 12);
		assertEq("12ab", 12 + "ab");
		assertEq("ab1.5", "ab" + 1.5);
		assertEq("1.5ab", 1.5 + "ab");
		assertEq("abtrue", "ab" + true);
		assertEq("falseab", false + "ab");
		assertEq("abnull", "ab" + null);
		assertEq("nullab", null + "ab");
	},
	{
		assertEq("", "".substring(0));
		assertEq("", "".substring(0, 0));
		assertEq("", "A".substring(0, 0));
		assertEq("", "A".substring(1, 1));
		assertEq("", "A".substring(1));
		assertEq("CDE", "ABCDE".substring(2));
		assertEq("CD", "ABCDE".substring(2, 4));
	},
	{
		assertEq(-1, "".index_of('A'));
		assertEq(-1, "ABCDEF".index_of('5'));
		assertEq(3, "ABCDEF".index_of('D'));
		assertEq(7, "Hello World".index_of('o', 5));
	},
	{
		var err = false;
		try {
			"".substring(1);
		} catch (e) { err = true; }
		assert(err);
	},
	{
		var err = false;
		try {
			"".substring(1, 2);
		} catch (e) { err = true; }
		assert(err);
	},
	{
		var obj = new (class { const x = 5; })();
		var err = false;
		try {
			obj.x = 10;
		} catch (e) { err = true; }
		assert(err);
	},
	{
		var obj = new (class { function x(){} })();
		var err = false;
		try {
			obj.x = 10;
		} catch (e) { err = true; }
		assert(err);
	},
	{
		var obj = new (class { public var x = 5; })();
		assertEq(5, obj.x);
		obj.x = 10;
		assertEq(10, obj.x);
	},
	{
		var err = false;
		try {
			var zz;
			zz;
		} catch (e) { err = true; }
		assert(err);
	},
	{
		var zz = 123;
		zz;
	},
	{//Unified file paths.
		assertEq("/a/b/c/d/e.f", new sys.File("\\a\\b\\c\\d\\e.f").get_path());
		assertEq("/a/b/c/d/e.f", new sys.File("\\a/b\\c/d\\e.f").get_path());

		if (sys.windows) {
			assertEq("\\a\\b\\c.d", new sys.File("/a/b/c.d").get_native_path());
		} else {
			assertEq("/a/b/c.d", new sys.File("\\a\\b\\c.d").get_native_path());
		}
	},
	{
		assertEq("a.txt", new sys.File("a.txt").get_name());
		assertEq("a.txt", new sys.File("/a.txt").get_name());
		assertEq("a.txt", new sys.File("/folder/a.txt").get_name());
		assertEq("folder", new sys.File("folder").get_name());
		assertEq("folder", new sys.File("folder/").get_name());
		assertEq("folder", new sys.File("parent/folder/").get_name());
		assertEq("folder", new sys.File("parent/folder").get_name());

		if (sys.windows) {
			assertEq("a.txt", new sys.File("c:/a.txt").get_name());
			assertEq("a.txt", new sys.File("c:a.txt").get_name());
			assertEq("a.txt", new sys.File("c:/folder/a.txt").get_name());
			assertEq("a.txt", new sys.File("c:folder/a.txt").get_name());
			assertEq("folder", new sys.File("c:/folder").get_name());
			assertEq("folder", new sys.File("c:/folder/").get_name());
			assertEq("folder", new sys.File("c:/parent/folder").get_name());
			assertEq("folder", new sys.File("c:/parent/folder/").get_name());
			assertEq("folder", new sys.File("c:folder/").get_name());
			assertEq("folder", new sys.File("c:parent/folder/").get_name());
			assertEq("folder", new sys.File("c:parent/folder").get_name());
			assertEq("", new sys.File("c:").get_name());
			assertEq("", new sys.File("c:/").get_name());
		}
	},
	{
		assertEq(null, new sys.File("a.txt").get_parent_path());
		assertEq("/", new sys.File("/a.txt").get_parent_path());
		assertEq("/folder", new sys.File("/folder/a.txt").get_parent_path());
		assertEq(null, new sys.File("folder").get_parent_path());
		assertEq(null, new sys.File("folder/").get_parent_path());
		assertEq("parent", new sys.File("parent/folder/").get_parent_path());
		assertEq("parent", new sys.File("parent/folder").get_parent_path());
		assertEq(null, new sys.File("/").get_parent_path());

		if (sys.windows) {
			assertEq("c:/", new sys.File("c:/a.txt").get_parent_path());
			assertEq("c:", new sys.File("c:a.txt").get_parent_path());
			assertEq("c:/folder", new sys.File("c:/folder/a.txt").get_parent_path());
			assertEq("c:folder", new sys.File("c:folder/a.txt").get_parent_path());
			assertEq("c:/", new sys.File("c:/folder").get_parent_path());
			assertEq("c:/", new sys.File("c:/folder/").get_parent_path());
			assertEq("c:/parent", new sys.File("c:/parent/folder").get_parent_path());
			assertEq("c:/parent", new sys.File("c:/parent/folder/").get_parent_path());
			assertEq("c:", new sys.File("c:folder/").get_parent_path());
			assertEq("c:parent", new sys.File("c:parent/folder/").get_parent_path());
			assertEq("c:parent", new sys.File("c:parent/folder").get_parent_path());
			assertEq(null, new sys.File("c:").get_parent_path());
			assertEq(null, new sys.File("c:/").get_parent_path());
		}
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		var out = file.text_out();
		try {
			out.println("Hello");
			out.println("World");
		} finally { out.close(); }

		var text = file.read_text();
		assertEq("Hello\nWorld\n", text);

		out = file.text_out(true);
		try {
			out.println("And bye.");
		} finally { out.close(); }

		text = file.read_text();
		assertEq("Hello\nWorld\nAnd bye.\n", text);

		file.delete();
		assert(!file.exists());
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		var out = file.text_out(true);
		try {
			out.println("Hello");
			out.println("World");
		} finally { out.close(); }

		var text = file.read_text();
		assertEq("Hello\nWorld\n", text);

		file.delete();
		assert(!file.exists());
	},
	{
		var bs = new sys.Bytes(0);
		assert(bs != null);
		assertEq(0, bs.length);
	},
	{
		var bs = new sys.Bytes(5);
		assertEq(5, bs.length);
		assertEq(0, bs[0]);
		assertEq(0, bs[1]);
		assertEq(0, bs[2]);
		assertEq(0, bs[3]);
		assertEq(0, bs[4]);
		bs[2] = 123;
		assertEq(123, bs[2]);
		bs[3] = 255;
		assertEq(255, bs[3]);
		bs[2] = 1;
		assertEq(1, bs[2]);
	},
	{
		function is_prime(v, cnt, primes) {
			for (var i = 0; i < cnt; ++i) {
				var a = primes[i];
				if (a * a > v) break;
				if (v % a == 0) return false;
			}
			return true;
		}

		function prime(max_cnt) {
			var primes = new[max_cnt];
			var v = 2;
			var cnt = 0;
			while (cnt < max_cnt) {
				if (is_prime(v, cnt, primes)) primes[cnt++] = v;
				++v;
			}
			return primes;
		}

		var p = prime(100);
		assertEq(100, p.length);

		var correct = [
			2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83,
			89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179,
			181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277,
			281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389,
			397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499,
			503, 509, 521, 523, 541
		];

		for (var i = 0; i < 100; ++i) assertEq(correct[i], p[i]);
	},
	{
		function fibonacci(fibonacci_cnt) {
			function int_to_bigint(v) {
				var len = 0;
				var t = v;
				while (t != 0) {
					t /= 10;
					++len;
				}

				var r = new[len];
				var ofs = 0;
				while (ofs < len) {
					r[ofs] = v % 10;
					v /= 10;
					++ofs;
				}

				return r;
			}

			function bigint_to_str(v) {
				var len = v.length;
				while (len > 0 && v[len - 1] == 0) --len;
				if (len == 0) return "0";

				var s = "";
				for (var i = 0; i < len; ++i) s = v[i] + s;
				return s;
			}

			function bigint_add(a, b) {
				var a_len = a.length;
				while (a_len > 0 && a[a_len - 1] == 0) --a_len;
				var b_len = b.length;
				while (b_len > 0 && b[b_len - 1] == 0) --b_len;

				if (a_len == 0) return b;
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
			;

			var a = int_to_bigint(0);
			var b = int_to_bigint(1);

			var result = new [fibonacci_cnt];
			result[0] = bigint_to_str(b);

			for (var i = 1; i < fibonacci_cnt; ++i) {
				var c = bigint_add(a, b);
				result[i] = bigint_to_str(c);
				a = b;
				b = c;
			}

			return result;
		}

		var f = fibonacci(100);
		assertEq(100, f.length);

		var correct = [
			"1", "1", "2", "3", "5", "8", "13", "21", "34", "55", "89", "144", "233", "377", "610",
			"987", "1597", "2584", "4181", "6765", "10946", "17711", "28657", "46368", "75025",
			"121393", "196418", "317811", "514229", "832040", "1346269", "2178309", "3524578",
			"5702887", "9227465", "14930352", "24157817", "39088169", "63245986", "102334155",
			"165580141", "267914296", "433494437", "701408733", "1134903170", "1836311903",
			"2971215073", "4807526976", "7778742049", "12586269025", "20365011074", "32951280099",
			"53316291173", "86267571272", "139583862445", "225851433717", "365435296162",
			"591286729879", "956722026041", "1548008755920", "2504730781961", "4052739537881",
			"6557470319842", "10610209857723", "17167680177565", "27777890035288", "44945570212853",
			"72723460248141", "117669030460994", "190392490709135", "308061521170129",
			"498454011879264", "806515533049393", "1304969544928657", "2111485077978050",
			"3416454622906707", "5527939700884757", "8944394323791464", "14472334024676221",
			"23416728348467685", "37889062373143906", "61305790721611591", "99194853094755497",
			"160500643816367088", "259695496911122585", "420196140727489673", "679891637638612258",
			"1100087778366101931", "1779979416004714189", "2880067194370816120", "4660046610375530309",
			"7540113804746346429", "12200160415121876738", "19740274219868223167",
			"31940434634990099905", "51680708854858323072", "83621143489848422977",
			"135301852344706746049", "218922995834555169026", "354224848179261915075"
		];

		for (var i = 0; i < 100; ++i) assertEq(correct[i], f[i]);
	},
	{
		function circle(r) {
			var hr = r * 2;
			var vr = r;
			var hs = (hr + 2) * 2;
			var vs = (vr + 2) * 2;

			var matrix = new[vs];
			for (var i = 0; i < vs; ++i) {
				var row = new[hs];
				matrix[i] = row;
				for (var j = 0; j < hs; ++j) row[j] = 0;
			}

			var cx = hr + 2;
			var cy = vr + 2;

			var x = 0;
			var y = vr;
			var ex_d = hr * hr;

			while (y >= 0) {
				matrix[cy + y][cx + x] = 1;
				matrix[cy - 1 - y][cx - 1 - x] = 1;
				matrix[cy + y][cx - 1 - x] = 1;
				matrix[cy - 1 - y][cx + x] = 1;
				var sy = hr * y / vr;
				var d = x * x + sy * sy;
				if (d <= ex_d) {
					++x;
				} else {
					--y;
				}
			}

			var result = new [matrix.length];
			for (var i = 0; i < matrix.length; ++i) {
				var s = "";
				for (var col : matrix[i]) s += (col == 0 ? "." : "o");
				result[i] = s;
			}

			return result;
		}

		var correct = [
			"....................................................................................",
			"........................................oooo........................................",
			"............................ooooooooooooo..ooooooooooooo............................",
			".......................oooooo..........................oooooo.......................",
			"...................ooooo....................................ooooo...................",
			"................oooo............................................oooo................",
			"..............ooo..................................................ooo..............",
			"............ooo......................................................ooo............",
			"..........ooo..........................................................ooo..........",
			"........ooo..............................................................ooo........",
			".......oo..................................................................oo.......",
			"......oo....................................................................oo......",
			".....oo......................................................................oo.....",
			"....oo........................................................................oo....",
			"...oo..........................................................................oo...",
			"..oo............................................................................oo..",
			"..o..............................................................................o..",
			".oo..............................................................................oo.",
			".o................................................................................o.",
			".o................................................................................o.",
			".o................................................................................o.",
			"oo................................................................................oo",
			"oo................................................................................oo",
			".o................................................................................o.",
			".o................................................................................o.",
			".o................................................................................o.",
			".oo..............................................................................oo.",
			"..o..............................................................................o..",
			"..oo............................................................................oo..",
			"...oo..........................................................................oo...",
			"....oo........................................................................oo....",
			".....oo......................................................................oo.....",
			"......oo....................................................................oo......",
			".......oo..................................................................oo.......",
			"........ooo..............................................................ooo........",
			"..........ooo..........................................................ooo..........",
			"............ooo......................................................ooo............",
			"..............ooo..................................................ooo..............",
			"................oooo............................................oooo................",
			"...................ooooo....................................ooooo...................",
			".......................oooooo..........................oooooo.......................",
			"............................ooooooooooooo..ooooooooooooo............................",
			"........................................oooo........................................",
			"...................................................................................."
		];

		var c = circle(20);
		assertEq(correct.length, c.length);
		for (var i = 0; i < correct.length; ++i) assertEq(correct[i], c[i]);
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();

		assert(!file.exists());
		assert(!file.is_file());
		assert(!file.is_directory());

		file.write_text("Hello!");
		assert(file.exists());
		assert(file.is_file());
		assert(!file.is_directory());
		assertEq("Hello!", file.read_text());
		assertEq(6, file.get_size());

		var file2 = new sys.File("testfile2.txt");
		if (file2.exists()) file2.delete();
		assert(!file2.exists());
		assert(!file2.is_file());

		file.rename_to(file2);
		assert(!file.exists());
		assert(!file.is_file());
		assert(file2.exists());
		assert(file2.is_file());
		assertEq(6, file2.get_size());
		assertEq("Hello!", file2.read_text());

		file2.delete();
		assert(!file2.exists());
		assert(!file2.is_file());
	},
	{
		function delete_recursively(file) {
			if (file.is_directory()) {
				for (var subfile : file.list_files()) delete_recursively(subfile);
				file.delete();
			} else if (file.exists()) {
				file.delete();
			}
		}

		var dir = new sys.File("testdir");
		delete_recursively(dir);
		assert(!dir.exists());
		assert(!dir.is_file());
		assert(!dir.is_directory());

		dir.mkdir();
		assert(dir.exists());
		assert(!dir.is_file());
		assert(dir.is_directory());

		var dir2 = new sys.File("testdir2");
		delete_recursively(dir2);
		assert(!dir2.exists());
		assert(!dir2.is_directory());

		dir.rename_to(dir2);
		assert(!dir.exists());
		assert(!dir.is_directory());
		assert(dir2.exists());
		assert(dir2.is_directory());

		dir2.delete();
		assert(!dir2.exists());
		assert(!dir2.is_directory());
	},
	{
		function delete_recursively(file) {
			if (file.is_directory()) {
				for (var subfile : file.list_files()) delete_recursively(subfile);
				file.delete();
			} else if (file.exists()) {
				file.delete();
			}
		}

		var dir = new sys.File("testdir");
		delete_recursively(dir);
		assert(!dir.exists());

		assertEq(0, dir.list_files().length);
		dir.mkdir();
		assert(dir.is_directory());
		assertEq(0, dir.list_files().length);

		var file = new sys.File(dir, "file.txt");
		assert(!file.exists());
		file.write_text("Hello World");
		assert(file.is_file());

		var files = dir.list_files();
		assertEq(1, files.length);
		assertEq("file.txt", files[0].get_name());
		file.delete();

		var subdir = new sys.File(dir, "subdir");
		assert(!subdir.exists());
		subdir.mkdir();
		assert(subdir.is_directory());
		files = dir.list_files();
		assertEq(1, files.length);
		assertEq("subdir", files[0].get_name());
		subdir.delete();

		dir.delete();
		assert(!dir.exists());
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("");
		assertEq(0, file.get_size());

		var in = file.binary_in();
		try {
			assertEq(-1, in.read_byte());
			var buffer = new sys.Bytes(4);
			assertEq(-1, in.read(buffer));
		} finally { in.close(); }

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello!");
		assertEq(6, file.get_size());

		var in = file.binary_in();
		try {
			assertEq('H', in.read_byte());
			assertEq('e', in.read_byte());
			assertEq('l', in.read_byte());
			assertEq('l', in.read_byte());
			assertEq('o', in.read_byte());
			assertEq('!', in.read_byte());
			assertEq(-1, in.read_byte());
		} finally { in.close(); }

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello!");
		assertEq(6, file.get_size());

		var buffer = new sys.Bytes(16);
		var in = file.binary_in();
		try {
			assertEq(6, in.read(buffer));
			assertEq('H', buffer[0]);
			assertEq('e', buffer[1]);
			assertEq('l', buffer[2]);
			assertEq('l', buffer[3]);
			assertEq('o', buffer[4]);
			assertEq('!', buffer[5]);
			assertEq(-1, in.read(buffer));
			assertEq('H', buffer[0]);
			assertEq('e', buffer[1]);
			assertEq('l', buffer[2]);
			assertEq('l', buffer[3]);
			assertEq('o', buffer[4]);
			assertEq('!', buffer[5]);
		} finally { in.close(); }

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello!");
		assertEq(6, file.get_size());

		var buffer = new sys.Bytes(16);
		var in = file.binary_in();
		try {
			assertEq(6, in.read(buffer));
			assertEq(-1, in.read_byte());
		} finally { in.close(); }

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello!");
		assertEq(6, file.get_size());

		var buffer = new sys.Bytes(16);
		var in = file.binary_in();
		try {
			assertEq('H', in.read_byte());
			assertEq('e', in.read_byte());
			assertEq('l', in.read_byte());
			assertEq('l', in.read_byte());
			assertEq('o', in.read_byte());
			assertEq('!', in.read_byte());
			assertEq(-1, in.read(buffer));
		} finally { in.close(); }

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		var out = file.binary_out();
		try {
			var str = "Hello!";
			for (var i = 0; i < str.length(); ++i) out.write_byte(str[i]);
		} finally { out.close(); }

		assertEq("Hello!", file.read_text());

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		var buffer = new sys.Bytes(16);
		var str = "Hello!";
		for (var i = 0; i < str.length(); ++i) buffer[i] = str[i];

		var out = file.binary_out();
		try {
			out.write(buffer, 0, str.length());
		} finally { out.close(); }

		assertEq("Hello!", file.read_text());

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello ");

		var out = file.binary_out();
		try {
		var str = "World";
			for (var i = 0; i < str.length(); ++i) out.write_byte(str[i]);
		} finally { out.close(); }

		assertEq("World", file.read_text());

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello ");

		var out = file.binary_out(true);
		try {
		var str = "World";
			for (var i = 0; i < str.length(); ++i) out.write_byte(str[i]);
		} finally { out.close(); }

		assertEq("Hello World", file.read_text());

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello ");
		file.write_text("World");
		assertEq("World", file.read_text());

		file.delete();
	},
	{
		var file = new sys.File("testfile.txt");
		if (file.exists()) file.delete();
		assert(!file.exists());

		file.write_text("Hello ");
		file.write_text("World", true);
		assertEq("Hello World", file.read_text());

		file.delete();
	},
	{
		var g_x = 5;

		function fn() { return g_x++; }
		function foo() { return "foo()"; }
		function bar() { return q; }
	
		var q = "hello";
	
		assertEq(5, fn());
		assertEq(6, g_x);
		assertEq("foo()", foo());
		assertEq(q, bar());
	},
	{
		var a = 0;
		function foo(tag) {
			var b = 100;
			return { var s = "" + tag + " " + a + " " + b; ++a; ++b; return s; };
		}
	
		var f1 = foo("f1");
		assertEq("f1 0 100", f1());
		var f2 = foo("f2");
		assertEq("f2 1 100", f2());
		assertEq("f1 2 101", f1());
		assertEq("f2 3 101", f2());
	},
	{
		var a = 0;
		function foo(tag) {
			var b = 100;
			var cls = class {
				var c = 500;
				function f() {
					var s = "" + tag + " " + a + " " + b + " " + c;
					++a;
					++b;
					++c;
					return s;
				}
			};
			return new cls();
		}

		var obj1 = foo("obj1");
		var obj2 = foo("obj2");
		var obj3 = foo("obj3");
		assertEq("obj1 0 100 500", obj1.f());
		assertEq("obj2 1 100 500", obj2.f());
		assertEq("obj3 2 100 500", obj3.f());
		assertEq("obj1 3 101 501", obj1.f());
		assertEq("obj2 4 101 501", obj2.f());
		assertEq("obj3 5 101 501", obj3.f());
		assertEq("obj1 6 102 502", obj1.f());
		assertEq("obj2 7 102 502", obj2.f());
		assertEq("obj3 8 102 502", obj3.f());
	},
	{
		var map = new sys.HashMap();
		assertEq(0, map.size());
		assertEq(true, map.is_empty());
		assertEq(false, map.contains("a"));
		assertEq(false, map.contains("b"));
		assertEq(null, map.get("a"));
		assertEq(null, map.put("a", 10));
		assertEq(1, map.size());
		assertEq(false, map.is_empty());
		assertEq(true, map.contains("a"));
		assertEq(false, map.contains("b"));
		assertEq(10, map.get("a"));
		assertEq(null, map.get("b"));
		assertEq(10, map.put("a", 20));
		assertEq(20, map.get("a"));
		map.clear();
		assertEq(0, map.size());
		assertEq(true, map.is_empty());
		assertEq(false, map.contains("a"));
		assertEq(false, map.contains("b"));
	},
	{
		var map = new sys.HashMap();
		for (var i = 0; i < 500; ++i) {
			map.put(i, i * i);
			map.put("k" + i, i * 33);
		}

		assertEq(1000, map.size());

		for (var i = 0; i < 500; ++i) {
			assertEq(i * i, map.get(i));
			assertEq(i * 33, map.get("k" + i));
		}
	},
	{
		var map = new sys.HashMap();
		map.put("a", 1);
		map.put("b", 13);
		map.put("c", 8);
		map.put("d", 29);
		map.put("e", 91);

		var keys = map.keys();
		assertEq(5, keys.length);
		keys.sort();
		assertEq("[a, b, c, d, e]", "" + keys);
		
		var values = map.values();
		assertEq(5, values.length);
		values.sort();
		assertEq("[1, 8, 13, 29, 91]", "" + values);
	},
	{//sys.execute()
		var scope = new sys.HashMap();
		scope.put("x", 5);
		scope.put("y", 10);
		var s = sys.execute("filename.ext", "return \"x = \" + x + \", y = \" + y;", scope);
		assertEq("x = 5, y = 10", s);
	},
	{//sys.execute_ex()
		var list = new sys.ArrayList();
		var scope = new sys.HashMap();
		scope.put("list", list);

		var scripts = [
			[ "file1.txt", "var f = 0; function foo(){ list.add(\"foo()=\" + (f++)); } list.add(\"file1.txt\"); foo(); return 111;" ],
			[ "file2.txt", "function bar(){ list.add(\"bar()\"); } list.add(\"file2.txt\"); foo(); bar(); return 222;" ],
			[ "file3.txt", "function fn3(){ list.add(\"fn3()\"); } list.add(\"file3.txt\"); foo(); bar(); fn3(); return 333;" ]
		];

		var s = sys.execute_ex(scripts, scope);
		assertEq(333, s);
		assertEq("[file1.txt, foo()=0, file2.txt, foo()=1, bar(), file3.txt, foo()=2, bar(), fn3()]", "" + list);
	},
	{
		var buf = new sys.StringBuffer();
		assertEq(true, buf.is_empty());
		assertEq(0, buf.length());
		assertEq("", buf.to_string());

		buf.append_char('H');
		buf.append_char('e');
		buf.append_char('l');
		buf.append_char('l');
		buf.append_char('o');
		buf.append_char('!');

		assertEq(false, buf.is_empty());
		assertEq(6, buf.length());
		assertEq("Hello!", buf.to_string());

		buf.clear();

		assertEq(true, buf.is_empty());
		assertEq(0, buf.length());
		assertEq("", buf.to_string());
	},
	{
		var buf = new sys.StringBuffer();
		buf.append("Hello ");
		buf.append("World");
		assertEq("Hello World", buf.to_string());
	},
	{
		var buf = new sys.StringBuffer();
		buf.append("Hello!");
		assertEq('H', buf[0]);
		assertEq('e', buf[1]);
		assertEq('l', buf[2]);
		assertEq('l', buf[3]);
		assertEq('o', buf[4]);
		assertEq('!', buf[5]);
	},
	{
		var buf = new sys.StringBuffer();
		for (var i = 0; i < 1000; ++i) {
			buf.append_char(i % 128);
			assertEq(i + 1, buf.length());
		}

		var s = buf.to_string();
		for (var i = 0; i < 1000; ++i) {
			assertEq(i % 128, s[i]);
		}
	},
	{
		var list = new sys.ArrayList();
		assertEq(true, list.is_empty());
		assertEq(0, list.size());
		assertEq(false, list.contains("Hello"));
		assertEq(-1, list.index_of("Hello"));
		assertEq("[]", "" + list.to_array());
		assertEq("[]", "" + list);

		list.add("Hello");
		assertEq(false, list.is_empty());
		assertEq(1, list.size());
		assertEq(true, list.contains("Hello"));
		assertEq(false, list.contains("Bye"));
		assertEq(0, list.index_of("Hello"));
		assertEq("Hello", list.get(0));
		assertEq("Hello", list[0]);
		list[0] = "Bye";
		assertEq(false, list.contains("Hello"));
		assertEq(-1, list.index_of("Hello"));
		assertEq(true, list.contains("Bye"));
		assertEq(0, list.index_of("Bye"));
		assertEq("Bye", list.get(0));
		assertEq("Bye", list[0]);
		assertEq("[Bye]", "" + list);

		list.add("World");
		assertEq(false, list.is_empty());
		assertEq(2, list.size());
		assertEq(false, list.contains("Hello"));
		assertEq(-1, list.index_of("Hello"));
		assertEq(true, list.contains("Bye"));
		assertEq(0, list.index_of("Bye"));
		assertEq(true, list.contains("World"));
		assertEq(1, list.index_of("World"));
		assertEq("Bye", list[0]);
		assertEq("World", list[1]);
		assertEq("[Bye, World]", "" + list.to_array());
		assertEq("[Bye, World]", "" + list);

		list.remove(0);
		assertEq(false, list.is_empty());
		assertEq(1, list.size());
		assertEq(false, list.contains("Bye"));
		assertEq(-1, list.index_of("Bye"));
		assertEq(true, list.contains("World"));
		assertEq(0, list.index_of("World"));
		assertEq("World", list[0]);
		assertEq("[World]", "" + list);

		list.clear();
		assertEq(true, list.is_empty());
		assertEq(0, list.size());
		assertEq(false, list.contains("World"));
		assertEq(-1, list.index_of("World"));
		assertEq("[]", "" + list);
	},
	{
		var list = new sys.ArrayList();
		for (var i = 0; i < 1000; ++i) list.add(i * i);

		assertEq(1000, list.size());
		var array = list.to_array();
		assertEq(1000, array.length);
		for (var i = 0; i < 1000; ++i) {
			assertEq(i * i, list[i]);
			assertEq(i * i, array[i]);
		}
	},
	{
		var list = new sys.ArrayList();
		for (var i = 0; i < 100; ++i) list.add(i * i);

		var i = 0;
		for (var k : list) {
			assertEq(i * i, k);
			++i;
		}
	},
	{
		var set = new sys.HashSet();
		assertEq(true, set.is_empty());
		assertEq(0, set.size());
		assertEq(false, set.contains("Hello"));
		assertEq("[]", "" + set.to_array());
		assertEq("[]", "" + set);

		assertEq(true, set.add("Hello"));
		assertEq(false, set.is_empty());
		assertEq(1, set.size());
		assertEq(true, set.contains("Hello"));
		assertEq(false, set.contains("Bye"));
		assertEq("[Hello]", "" + set.to_array());
		assertEq("[Hello]", "" + set);

		assertEq(true, set.add("Bye"));
		assertEq(false, set.is_empty());
		assertEq(2, set.size());
		assertEq(true, set.contains("Hello"));
		assertEq(true, set.contains("Bye"));

		assertEq(false, set.remove("World"));
		assertEq(2, set.size());
		assertEq(true, set.contains("Hello"));
		assertEq(true, set.contains("Bye"));

		assertEq(true, set.remove("Hello"));
		assertEq(1, set.size());
		assertEq(false, set.contains("Hello"));
		assertEq(true, set.contains("Bye"));
		assertEq("[Bye]", "" + set.to_array());
		assertEq("[Bye]", "" + set);

		set.clear();
		assertEq(true, set.is_empty());
		assertEq(0, set.size());
		assertEq(false, set.contains("Hello"));
		assertEq("[]", "" + set.to_array());
		assertEq("[]", "" + set);
	},
	{
		var set = new sys.HashSet();
		for (var i = 0; i < 100; ++i) {
			assertEq(true, set.add(i));
		}

		for (var i = 0; i < 100; ++i) {
			assertEq(true, set.contains(i));
			assertEq(false, set.contains(i + 100));
		}

	},
	{
		var set = new sys.HashSet();
		for (var i = 0; i < 100; ++i) set.add(i * i);
		var array = set.to_array();
		assertEq(100, array.length);

		array.sort();
		for (var i = 0; i < 100; ++i) {
			assertEq(true, set.contains(i * i));
			assertEq(i * i, array[i]);
		}
	},
	{//Array.sort()
		var array = [ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 ];
		array.sort();
		assertEq("[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]", "" + array);
		array = [ 2, 1, 0, 5, 4, 3, 8, 7, 6 ];
		array.sort();
		assertEq("[0, 1, 2, 3, 4, 5, 6, 7, 8]", "" + array);
	},
	{//ArrayList.sort()
		function array_to_list(a) {
			var lst = new sys.ArrayList();
			for (var v : a) lst.add(v);
			return lst;
		}

		var list = array_to_list([ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 ]);
		list.sort();
		assertEq("[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]", "" + list);
		list = array_to_list([ 2, 1, 0, 5, 4, 3, 8, 7, 6 ]);
		list.sort();
		assertEq("[0, 1, 2, 3, 4, 5, 6, 7, 8]", "" + list);
	},
	{//Big sort.
		function sort(a, start, end) {
			const len = end - start;

			function merge(middle) {
				var res = new[len];
				var ofs1 = start;
				var end1 = middle;
				var ofs2 = middle;
				var end2 = end;
				var dst = 0;

				while (ofs1 < end1 && ofs2 < end2) {
					var v;
					var v1 = a[ofs1];
					var v2 = a[ofs2];
					if (v1 <= v2) {
						v = v1;
						++ofs1;
					} else {
						v = v2;
						++ofs2;
					}
					res[dst++] = v;
				}
				while (ofs1 < end1) res[dst++] = a[ofs1++];
				while (ofs2 < end2) res[dst++] = a[ofs2++];

				for (var i = 0; i < len; ++i) a[start + i] = res[i];
			}

			function selection_sort() {
				for (var i = start, end_minus_1 = end - 1; i < end_minus_1; ++i) {
					var min_i = i;
					var min = a[i];
					for (var j = i + 1; j < end; ++j) {
						var v = a[j];
						if (v < min) {
							min = v;
							min_i = j;
						}
					}
					if (min_i != i) {
						a[min_i] = a[i];
						a[i] = min;
					}
				}
			}

			if (len > 10) {
				var middle = start + len / 2;
				sort(a, start, middle);
				sort(a, middle, end);
				merge(middle);
			} else if (len > 1) {
				selection_sort();
			}
		}

		const N = 1000;
		var array = new[N];
		for (var i = 0, r = 123; i < N; ++i, r = r * 1664525 + 1013904223) array[i] = r % N;

		var copy = new[N];
		for (var i = 0; i < N; ++i) copy[i] = array[i];

		array.sort();
		sort(copy, 0, N);

		assertEq("" + copy, "" + array);
	},
	{
		assertEq(false, "aaa" == "bbb");
		assertEq(true, "aaa" != "bbb");
		assertEq(true, "aaa" < "bbb");
		assertEq(true, "aaa" <= "bbb");
		assertEq(false, "aaa" > "bbb");
		assertEq(false, "aaa" >= "bbb");

		assertEq(true, "aaa" == "aaa");
		assertEq(false, "aaa" != "aaa");
		assertEq(false, "aaa" < "aaa");
		assertEq(true, "aaa" <= "aaa");
		assertEq(false, "aaa" > "aaa");
		assertEq(true, "aaa" >= "aaa");

		assertEq(false, "bbb" == "aaa");
		assertEq(true, "bbb" != "aaa");
		assertEq(false, "bbb" < "aaa");
		assertEq(false, "bbb" <= "aaa");
		assertEq(true, "bbb" > "aaa");
		assertEq(true, "bbb" >= "aaa");

		assertEq(false, "aaa" == "aaaa");
		assertEq(true, "aaa" != "aaaa");
		assertEq(true, "aaa" < "aaaa");
		assertEq(true, "aaa" <= "aaaa");
		assertEq(false, "aaa" > "aaaa");
		assertEq(false, "aaa" >= "aaaa");
	},
	{//Access modifiers: default variable, from outside.
		class X { var k = 10; }
		var x = new X();
		try { var i = x.k; assert(false); } catch (e){}
		try { x.k = 5; assert(false); } catch (e){}
	},
	{//Access modifiers: public variable, from outside.
		class X { public var k = 10; }
		var x = new X();
		var i = x.k;
		x.k = 5;
	},
	{//Access modifiers: default constant, from outside.
		class X { const k = 10; }
		var x = new X();
		try { var i = x.k; assert(false); } catch (e){}
	},
	{//Access modifiers: public constant, from outside.
		class X { public const k = 10; }
		var x = new X();
		var i = x.k;
	},
	{//Access modifiers: default function, from outside.
		class X { function f(){} }
		var x = new X();
		x.f();
	},
	{//Access modifiers: private function, from outside.
		class X { private function f(){} }
		var x = new X();
		try { x.f(); assert(false); } catch (e){}
	},
	{//Access modifiers: default class, from outside.
		class X { class Z{} }
		var x = new X();
		try { new x.Z(); assert(false); } catch (e){}
	},
	{//Access modifiers: public class, from outside.
		class X { public class Z{} }
		var x = new X();
		new x.Z();
	},
	{//Access modifiers: private variable, from inside.
		class X { private var k = 10; function fn(){ return k; }  }
		var x = new X();
		x.fn();
	},
	{//Access modifiers: private constant, from inside.
		class X { const k = 10; function fn(){ return k; } }
		var x = new X();
		x.fn();
	},
	{//Access modifiers: private function, from inside.
		class X { function k(){} function fn(){ return k(); } }
		var x = new X();
		x.fn();
	},
	{//Access modifiers: private class, from inside.
		class X { class Z{} function fn(){ return new Z(); } }
		var x = new X();
		x.fn();
	},
	{//sys.execute_ex(): first script depends on the second one.
		var scripts = [
			[ "file1.txt", "const Z = foo();" ],
			[ "file2.txt", "function foo() { return 123; } return Z;" ]
		];

		var s = sys.execute_ex(scripts, new sys.HashMap());
		assertEq(123, s);
	},
	{//break in for
		var list = new sys.ArrayList();
		for (var i = 0; i < 5; ++i) {
			list.add("A" + i);
			if (i == 2) break;
			list.add("B" + i);
		}
		assertEq("[A0, B0, A1, B1, A2]", "" + list);
	},
	{//break in for
		var list = new sys.ArrayList();
		for (var i = 0; i < 5; ++i) {
			list.add("A" + i);
			if (i == 2) {
				list.add("Z");
				break;
			}
			list.add("B" + i);
		}
		assertEq("[A0, B0, A1, B1, A2, Z]", "" + list);
	},
	{//break in for-each
		var list = new sys.ArrayList();
		for (var i : [0, 1, 2, 3, 4]) {
			list.add("A" + i);
			if (i == 2) break;
			list.add("B" + i);
		}
		list.add("Z");
		assertEq("[A0, B0, A1, B1, A2, Z]", "" + list);
	},
	{//break in for-each
		var list = new sys.ArrayList();
		function fn() {
			for (var i : [0, 1, 2, 3, 4]) {
				list.add("A" + i);
				if (i == 2) break;
				list.add("B" + i);
			}
			list.add("Z");
		}
		fn();
		assertEq("[A0, B0, A1, B1, A2, Z]", "" + list);
	},
	{//Default element in array.
		var a = new[5];
		assertEq(null, a[0]);
		assertEq(null, a[1]);
		assertEq(null, a[2]);
		assertEq(null, a[3]);
		assertEq(null, a[4]);
	},
	{//sys.str_to_int()
		assertEq(0, sys.str_to_int("0"));
		assertEq(12345, sys.str_to_int("12345"));
	},
	{//sys.str_to_int() : empty string
		var err = true;
		try {
			sys.str_to_int("");
			err = false;
		} catch (e){}
		assert(err);
	},
	{
		const a = 5;
		const b = 0;
		var err = true;
		try {
			var c = a / b;
			err = false;
		} catch (e){}
		assert(err);
	},
	{//Bug: wrong lexical error.
		sys.execute("foo.s", "/*foo\n*/");
	},
	{//Bug: fails on too big decimal integer literal.
		var s = sys.execute("foo.s", "var x = 10000000000; return \"\" + x;");
		assertEq("10000000000", s);
	},
	{//Bug: fails on too big hexadecimal integer literal.
		var s = sys.execute("foo.s", "var x = 0x1000000000; return \"\" + x;");
		assertEq("68719476736", s);
	}
];

var total_time = sys.current_time_millis();
var totalCnt = 0;
var failCnt = 0;
for (var i = 0; i < tests.length; ++i) {
	var ok = false;
	var error = null;
	var test_time = sys.current_time_millis();
	try {
		tests[i]();
		ok = true;
	} catch (e) {
		++failCnt;
		error = e;
	}
	++totalCnt;

	var test_duration = sys.current_time_millis() - test_time;
	sys.out.println("Test " + i + ": " + (ok ? "OK" : "*ERROR*") + ", " + test_duration + " ms");
	if (!ok) error.print();
}

var total_duration = sys.current_time_millis() - total_time;

sys.out.println();
sys.out.println("*****************************");
var message;
if (failCnt == 0) {
	message = "OK (" + totalCnt + ")";
} else {
	message = "FAILED: " + failCnt + "/" + totalCnt;
}
sys.out.println(message + ", " + total_duration + " ms");
sys.out.println("*****************************");
