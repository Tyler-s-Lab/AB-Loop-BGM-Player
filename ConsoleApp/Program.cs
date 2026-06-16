using PlayerKernel;

namespace ConsoleApp {
	internal class Program {

		private static readonly BgmWrapper bgm = new();

		static void Main(string[] args) {
			Console.WriteLine("Hello, World!");

			bgm.open(@"C:\Users\Myste\Desktop\m_sys_rouge5_svtheme1.ogg");
			bgm.play();

			Console.ReadLine();
		}
	}
}
