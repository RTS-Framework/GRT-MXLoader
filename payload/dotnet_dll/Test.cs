using System;

namespace dotnet_dll
{
    public class Test
    {
        public void Method0()
        {
            Console.WriteLine("Method0 is called");
            return;
        }

        public int Method1(string arg0, string arg1)
        {
            Console.WriteLine("Method1 is called");
            if (arg0 != "arg0")
            {
                Console.WriteLine("invalid argument 0");
                return 1;
            }
            if (arg1 != "arg1")
            {
                Console.WriteLine("invalid argument 1");
                return 1;
            }
            return 0;
        }
    }
}
