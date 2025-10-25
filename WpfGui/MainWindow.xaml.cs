using Microsoft.Win32;
using PlayerKernel;
using System.IO;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace WpfGui {
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window {

		private readonly DispatcherTimer timer;
		private bool isDragging = false;
		private readonly BgmWrapper bgm = new();
		private float m_duration = 0;

		public MainWindow() {
			InitializeComponent();

			timer = new DispatcherTimer {
				Interval = TimeSpan.FromSeconds(0.01)
			};
			timer.Tick += Timer_Tick;
		}

		private void Timer_Tick(object? sender, EventArgs e) {
			// 只有当用户没有拖动滑块时才更新进度
			if (!isDragging && bgm.isOpened()) {
				progressSlider.Value = bgm.getOffset();
				float off = bgm.getOffset();
				int sec = (int)float.Round(off * 10) / 10;
				int m = (int)float.Round(off * 10) % 10;
				timeCurrent.Text = string.Format("{0:d2}:{1:d2}.{2}", sec / 60, sec % 60, m);
			}
		}

		private void ProgressSlider_DragStarted(object sender, DragStartedEventArgs e) {
			isDragging = true; // 开始拖动，忽略进度更新
			timer.Stop(); // 暂停定时器，避免冲突
		}

		private void ProgressSlider_DragCompleted(object sender, DragCompletedEventArgs e) {
			isDragging = false;
			// 定位到滑块位置
			bgm.setOffset((float)progressSlider.Value);
			timer.Start(); // 恢复定时器
		}

		private void ProgressSlider_MouseDown(object sender, MouseButtonEventArgs e) {
			// 计算点击位置对应的值
			Point clickPoint = e.GetPosition(progressSlider);
			double percentage = clickPoint.X / progressSlider.ActualWidth;
			double newValue = percentage * (progressSlider.Maximum - progressSlider.Minimum);

			// 更新进度条和播放位置
			progressSlider.Value = newValue;
			bgm.setOffset((float)newValue);
		}

		// 在播放开始时启动定时器
		private void PlayMedia() {
			//MediaPlayer.Play();
		}

		private void Button_Click(object sender, RoutedEventArgs e) {
		}

		private void Button_Click_Play(object sender, RoutedEventArgs e) {
			bgm.play();

			timer.Start();
		}

		private void Button_Click_Pause(object sender, RoutedEventArgs e) {
			bgm.pause();

			timer.Stop();
		}

		private void Button_Click_Stop(object sender, RoutedEventArgs e) {
			bgm.stop();

			timer.Stop();

			timeCurrent.Text = "00:00.0";
			progressSlider.Value = 0;
		}

		private void Button_Click_Open(object sender, RoutedEventArgs e) {
			OpenFileDialog openFileDialog = new OpenFileDialog{
				Filter = "BGM|*.ogg;*.flac|All Files|*.*", // 设置文件类型过滤
				Multiselect = false // 是否允许多选
			};

			if (openFileDialog.ShowDialog() != true) {
				return;
			}
			OpenBgm(openFileDialog.FileName);
		}

		private void Window_Drop(object sender, DragEventArgs e) {
			if (e.Data.GetDataPresent(DataFormats.FileDrop)) {
				string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
				if (files.Length > 0) {
					OpenBgm(files[0]);
				}
			}
		}

		private void OpenBgm(string path) {
			try {
				bgm.open(path);
			}
			catch (Exception) {
				textBlock.Text = $"Failed to open '{path}'";
				return;
			}
			// 获取媒体的总时长，设置为进度条的最大值
			m_duration = bgm.getDuration();
			progressSlider.Maximum = m_duration;

			ulong lA = bgm.getLoopPointA();
			ulong lB = bgm.getLoopPointB();

			textBlock.Text = string.Format(
				"{0}\n\nLoop Points: {1:d2}:{2:d2}.{3:d3}.{7:d3} / {4:d2}:{5:d2}.{6:d3}.{8:d3}",
				path,
				lA / 1000000 / 60, lA / 1000000 % 60, lA / 1000 % 1000,
				lB / 1000000 / 60, lB / 1000000 % 60, lB / 1000 % 1000,
				lA % 1000, lB % 1000
			);

			{
				int sec = (int)float.Round(m_duration * 10) / 10;
				int m = (int)float.Round(m_duration * 10) % 10;
				timeDuration.Text = string.Format("{0:d2}:{1:d2}.{2}", sec / 60, sec % 60, m);
			}

			timeCurrent.Text = "00:00.0";
			progressSlider.Value = 0;
		}
	}
}
