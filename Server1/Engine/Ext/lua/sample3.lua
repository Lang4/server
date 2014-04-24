-- ���� g_test �� ��� ���� _test �� ���� ����Ѵ�.
print(g_test._test)

-- const char* test::is_test() �Լ��� ����� ����Ѵ�.
print(g_test:is_test())

-- test::ret_int() �Լ��� ������� ����Ѵ�.
print(g_test:ret_int())

-- temp �� �� test ��ü�� ����� �ִ´�.
temp = test(4)

-- test �� ��� ���� _test ���� ����Ѵ�.
print(temp._test)

-- Lua �� �߰����� ���� A ����ü���� a��� ������ �ִ´�.
a = g_test:get()

-- ������ ��ü a�� Lua->C++�� �����Ѵ�.
temp:set(a)

-- test::set(A a) �Լ� ȣ��� _test �� ���� ��ȭ�� Ȯ���Ѵ�.
print(temp._test)

-- ��ӹ��� �θ��� �Լ� ȣ��
print(temp:is_base())

-- �ڱ� �ڽ��� �Լ� ȣ��
print(temp:is_test())

-- ��� ��ü�� metatable�� ���� ��ϵ� Ŭ������ �� �Լ����� ���캸�� �Լ�
-------------------------------------------------------------------------------
function objinfo(obj)

	local meta = getmetatable(obj)
	if meta ~= nil then
		metainfo(meta)
	else
		print("no object infomation !!")
	end
end

function metainfo(meta)

	if meta ~= nil then
		local name = meta["__name"]
		if name ~= nil then
			metainfo(meta["__parent"])
			print("<"..name..">")
			for key,value in pairs(meta) do 
				if not string.find(key, "__..") then 
					if type(value) == "function" then
						print("\t[f] "..name..":"..key.."()") 
					elseif type(value) == "userdata" then
						print("\t[v] "..name..":"..key)
					end
				end
			end
		end
	end
end
-------------------------------------------------------------------------------

-- Lua ���� ��ü�� userdata�� �νĵȴ�.
print("g_test	-> ", g_test)
print("temp	-> ", temp)
print("a	-> ", a)

-- C++ ���� ����� g_test �� ��ü ������ ���캻��.
print("objinfo(g_test)")
objinfo(g_test)

-- constructor �� ���� ������ temp ��ü ������ ���캻��.
print("objinfo(temp)")
objinfo(temp)

-- ������� ���� A ����ü ���� a ��ü ������ ���캻��.
print("objinfo(a)")
objinfo(a)

