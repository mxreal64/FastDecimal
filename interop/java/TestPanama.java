import com.fastdecimal.FastDecimal;

public class TestPanama {
    public static void main(String[] args) {
        FastDecimal a = FastDecimal.parse("123.456");
        FastDecimal b = FastDecimal.parse("654.321");
        FastDecimal c = a.add(b);
        System.out.println("Java Panama test: a + b = " + c);
        if (!c.toString().equals("777.777")) {
            throw new AssertionError("Expected 777.777, got " + c);
        }

        FastDecimal p1 = FastDecimal.parse("10.5");
        FastDecimal p2 = FastDecimal.parse("2.0");
        FastDecimal mult = p1.multiply(p2);
        FastDecimal div = p1.divide(p2);
        System.out.println("Mult: " + mult + ", Div: " + div);
        if (!mult.toString().equals("21")) throw new AssertionError("mult failed");
        if (!div.toString().equals("5.25")) throw new AssertionError("div failed");

        System.out.println("Java Project Panama verification PASSED!");
    }
}
