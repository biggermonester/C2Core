using System;

public static class DummyDotnetExec
{
    public static void Main(string[] args)
    {
        Console.Write("dotnetexec-dummy");
        foreach (string arg in args)
        {
            Console.Write(" ");
            Console.Write(arg);
        }
        Console.WriteLine();
    }
}
