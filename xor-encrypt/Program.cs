using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;

namespace xor_encrypt
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                PrintUsage();
                return;
            }

            var inputFile = args[0];
            var outputFile = args[1];
            byte KEY = 0xAB;

            try
            {
                using(var input = File.OpenRead(inputFile))
                using(var output = File.OpenWrite(outputFile))
                {
                    byte[] buffer = new byte[1024 * 1024];
                    int read = 0;
                    while ((read = input.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        // xor transform the buffer
                        var transformed = buffer.Take(read).Select(b => (byte)(b ^ KEY)).ToArray();

                        // write transformed buffer to output
                        output.Write(transformed, 0, transformed.Length);
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("ERROR: " + ex.ToString());
            }
        }

        private static void PrintUsage()
        {
            Console.WriteLine("xor-encrypt <input-file> <output-file>");
        }
    }
}
