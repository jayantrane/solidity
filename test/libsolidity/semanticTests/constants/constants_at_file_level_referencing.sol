uint256 constant c = a.length + 56;
bytes constant a = b;
bytes constant b = hex"030102";

contract C {
    function f() public returns (bytes memory) {
        return a;
    }

    function g() public returns (bytes memory) {
        return b;
    }

    function h() public returns (bytes memory) {
        return bytes(c);
    }

    function i() public returns (uint, ActionChoices, bytes32) {
        return (x, choices, st);
    }
}

// ====
// compileViaYul: also
// ----
// f() -> 0x20, 3, "\x03\x01\x02"
// g() -> 0x20, 3, "\x03\x01\x02"
// h() -> 0x20, 5, "hello"
