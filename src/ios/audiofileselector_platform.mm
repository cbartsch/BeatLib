#include "audiofileselector.h"

#import <MediaPlayer/MediaPlayer.h>
#import <AVFoundation/AVFoundation.h>

struct AudioFileSelectorPrivate {
  void mediaItemSelected(MPMediaItem *mediaItem);

  AudioFileSelector *d;
};

struct AudioStreamPrivate {
  AudioStreamPrivate(QString name) : file(name) {
    file.open(QFile::OpenModeFlag::ReadOnly);
  }

  ~AudioStreamPrivate() {
    file.close();
  }

  QFile file;
};

@interface PickerDelegate : NSObject<MPMediaPickerControllerDelegate>

@property (nonatomic) AudioFileSelectorPrivate *d;

@end

@implementation PickerDelegate

@synthesize d;

- (void)mediaPicker:(MPMediaPickerController *)mediaPicker didPickMediaItems:(MPMediaItemCollection *)mediaItemCollection {
  Q_UNUSED(mediaPicker);
  if(mediaItemCollection.count > 0) {
    self.d->mediaItemSelected(mediaItemCollection.items[0]);
  } else {
    qWarning() << "MediaPickerController: user did not select any media items.";
  }
  [UIApplication.sharedApplication.keyWindow.rootViewController dismissViewControllerAnimated:YES completion:^{}];
}

- (void)mediaPickerDidCancel:(MPMediaPickerController *)mediaPicker {
  Q_UNUSED(mediaPicker);
  [UIApplication.sharedApplication.keyWindow.rootViewController dismissViewControllerAnimated:YES completion:^{}];
}

@end

QByteArray AudioStream::read(int numBytes) {
  return d->file.read(numBytes);
}

void AudioFileSelector::platform_init() {
  d = new AudioFileSelectorPrivate;
  d->d = this;
}

void AudioFileSelector::selectAudioFile()
{
  qWarning() << "AudioFileSelector::selectAudioFile() is not supported on iOS. Use selectAudioStream() instead.";
}

void AudioFileSelector::selectAudioStream()
{
  auto status = MPMediaLibrary.authorizationStatus;
  if(status == MPMediaLibraryAuthorizationStatusNotDetermined) {
    [MPMediaLibrary requestAuthorization:^(MPMediaLibraryAuthorizationStatus) {
      selectAudioStream();
    }];
  } else if(status == MPMediaLibraryAuthorizationStatusDenied) {
    qWarning() << "User did not authorize access to media library. Cannot open music file.";
  } else {
    auto controller = [[MPMediaPickerController alloc] initWithMediaTypes:MPMediaTypeMusic];
    auto delegate = [PickerDelegate new];
    delegate.d = d;
    controller.delegate = delegate;
    [UIApplication.sharedApplication.keyWindow.rootViewController presentViewController:controller animated:YES completion:^{}];
  }
}

void AudioFileSelectorPrivate::mediaItemSelected(MPMediaItem *mediaItem) {
  // the only way to read an MP3 file from the music library on iOS
  // is to export it to a file with AVAssetExportSession
  // somehow it needs to be saved as quicktime move (MOV)
  // fortunately it seems to actually be an MP3 file readable by MPG123

  if(!mediaItem.assetURL) {
    qWarning() << "MPMediaItem.assetURL == nil";
    return;
  }

  AVURLAsset *asset = [AVURLAsset URLAssetWithURL:static_cast<NSURL * _Nonnull>(mediaItem.assetURL) options:nil];

  AVAssetExportSession *exporter = [[AVAssetExportSession alloc] initWithAsset:asset presetName:AVAssetExportPresetPassthrough];

  exporter.outputFileType = @"com.apple.quicktime-movie";

  auto exportDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];

  NSString *exportFile = [exportDir stringByAppendingPathComponent:@"exported.mov"];

  QFile file(QString::fromNSString(exportFile));

  if(file.exists() && !file.remove()) {
    qWarning() << "Temp export file already exists and can't be deleted!";
  }

  NSURL *exportURL = [NSURL fileURLWithPath:exportFile];
  exporter.outputURL = exportURL;
  exporter.metadata = asset.metadata;

  [exporter exportAsynchronouslyWithCompletionHandler:^{
    qDebug() << "exporter status:" << exporter.status << exporter.error;

    if(exporter.status == AVAssetExportSessionStatusCompleted) {
      emit d->audioStreamSelected(new AudioStream(d, new AudioStreamPrivate(QString::fromNSString(exportFile))));
    }
  }];
}
