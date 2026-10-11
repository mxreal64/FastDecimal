using System;
using FastDecimal;

class Program
{
    static void Main()
    {
        var a = FastDecimal.Decimal.Parse("123.456");
        var b = FastDecimal.Decimal.Parse("654.321");
        var c = a + b;
        Console.WriteLine($"C# test: a + b = {c}");
        if (c.ToString() != "777.777") throw new Exception($"Expected 777.777, got {c}");

        var p1 = FastDecimal.Decimal.Parse("10.5");
        var p2 = FastDecimal.Decimal.Parse("2.0");
        var mult = p1 * p2;
        var div = p1 / p2;
        Console.WriteLine($"Mult: {mult}, Div: {div}");
        if (mult.ToString() != "21") throw new Exception("mult failed");
        if (div.ToString() != "5.25") throw new Exception("div failed");

        Console.WriteLine("C# Interop verification PASSED!");
    }
}
