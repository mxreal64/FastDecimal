use fastdecimal::Decimal;

#[test]
fn test_rust_interop() {
    let a = Decimal::parse("123.456");
    let b = Decimal::parse("654.321");
    let c = a + b;
    assert_eq!(c.to_string(), "777.777");

    let p1 = Decimal::parse("10.5");
    let p2 = Decimal::parse("2.0");
    assert_eq!((p1 * p2).to_string(), "21");
    assert_eq!((p1 / p2).to_string(), "5.25");

    let d = Decimal::from_i64(42);
    assert_eq!(d.to_i64(), 42);
}
