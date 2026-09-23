using System;

namespace dotnet_exe
{
    internal class Program
    {
        static int Main(string[] args)
        {
            if (args == null || args.Length == 0)
            {
                Console.WriteLine("PASS: no arguments");
                return 0;
            }

            if (args.Length != 4)
            {
                Console.WriteLine("invalid argument count");
                return 1;
            }

            if (args[0] != "-p1")
            {
                Console.WriteLine("invalid argument: 0");
                return 2;
            }
            if (args[1] != "123")
            {
                Console.WriteLine("invalid argument: 1");
                return 2;
            }
            if (args[2] != "-p2")
            {
                Console.WriteLine("invalid argument: 2");
                return 2;
            }
            if (args[3] != "abc")
            {
                Console.WriteLine("invalid argument: 3");
                return 2;
            }
            return 0;
        }
    }
}
