/**********************************************************************

Audacity: A Digital Audio Editor

ProjectAudioManager.h

Paul Licameli split from ProjectManager.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_AUDIO_MANAGER__
#define __AUDACITY_PROJECT_AUDIO_MANAGER__

#include <memory>
#include <vector>

#include "AudioIOListener.h" // to inherit
#include "ClientData.h" // to inherit
#include <wx/event.h> // to declare custom event type

constexpr int RATE_NOT_SELECTED{ -1 };

class AudacityProject;
struct AudioIOStartStreamOptions;
class TrackList;
class SelectedRegion;

class WaveTrack;
using WaveTrackArray = std::vector < std::shared_ptr < WaveTrack > >;

enum class PlayMode : int {
   normalPlay,
   oneSecondPlay, // Disables auto-scrolling
   loopedPlay, // Possibly looped play (not always); disables auto-scrolling
   cutPreviewPlay
};

struct TransportTracks;

enum StatusBarField : int;

struct RecordingDropoutEvent;
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_RECORDING_DROPOUT, RecordingDropoutEvent);

//! Notification, posted on the project, after recording has stopped, when dropouts have been detected
struct RecordingDropoutEvent : public wxCommandEvent
{
   //! Start time and duration
   using Interval = std::pair<double, double>;
   using Intervals = std::vector<Interval>;

   explicit RecordingDropoutEvent(const Intervals &intervals)
      : wxCommandEvent{ EVT_RECORDING_DROPOUT }
      , intervals{ intervals }
   {}

   RecordingDropoutEvent( const RecordingDropoutEvent& ) = default;

   wxEvent *Clone() const override {
      // wxWidgets will own the event object
      return safenew RecordingDropoutEvent(*this); }

   //! Disjoint and sorted increasingly
   const Intervals &intervals;
};

class AUDACITY_DLL_API ProjectAudioManager final
   : public ClientData::Base
   , public AudioIOListener
   , public std::enable_shared_from_this< ProjectAudioManager >
{
public:
   static ProjectAudioManager &Get( AudacityProject &project );
   static const ProjectAudioManager &Get( const AudacityProject &project );

   // Find suitable tracks to record into, or return an empty array.
   static WaveTrackArray ChooseExistingRecordingTracks(
      AudacityProject &proj, bool selectedOnly,
      double targetRate = RATE_NOT_SELECTED);

   static bool UseDuplex();

   static TransportTracks GetAllPlaybackTracks(
      TrackList &trackList, bool selectedOnly,
      bool nonWaveToo = false //!< if true, collect all PlayableTracks
   );

   explicit ProjectAudioManager( AudacityProject &project );
   ProjectAudioManager( const ProjectAudioManager & ) PROHIBITED;
   ProjectAudioManager &operator=( const ProjectAudioManager & ) PROHIBITED;
   ~ProjectAudioManager() override;

   bool IsTimerRecordCancelled() { return mTimerRecordCanceled; }
   void SetTimerRecordCancelled() { mTimerRecordCanceled = true; }
   void ResetTimerRecordCancelled() { mTimerRecordCanceled = false; }

   bool Paused() const { return mPaused; }

   bool Playing() const;

   // Whether recording into this project (not just into some project) is
   // active
   bool Recording() const;

   bool Stopping() const { return mStopping; }

   // Whether the last attempt to start recording requested appending to tracks
   bool Appending() const { return mAppending; }
   // Whether potentially looping play (using new default PlaybackPolicy)
   bool Looping() const { return mLooping; }
   bool Cutting() const { return mCutting; }

   // A project is only allowed to stop an audio stream that it owns.
   bool CanStopAudioStream () const;

   void OnRecord(bool altAppearance);

   bool DoRecord(AudacityProject &project,
      const TransportTracks &transportTracks, // If captureTracks is empty, then tracks are created
      double t0, double t1,
      bool altAppearance,
      const AudioIOStartStreamOptions &options);

   int PlayPlayRegion(const SelectedRegion &selectedRegion,
                      const AudioIOStartStreamOptions &options,
                      PlayMode playMode,
                      bool backwards = false);

   // Play currently selected region, or if nothing selected,
   // play from current cursor.
   void PlayCurrentRegion(
      bool newDefault = false, //!< See DefaultPlayOptions
      bool cutpreview = false);

   void OnPause();
   
   // Pause - used by AudioIO to pause sound activate recording
   void Pause();

   // Stop playing or recording
   void Stop(bool stopStream = true);

   void StopIfPaused();

   bool DoPlayStopSelect( bool click, bool shift );
   void DoPlayStopSelect( );

   PlayMode GetLastPlayMode() const { return mLastPlayMode; }

private:
   void SetPaused( bool value ) { mPaused = value; }
   void SetAppending( bool value ) { mAppending = value; }
   void SetLooping( bool value ) { mLooping = value; }
   void SetCutting( bool value ) { mCutting = value; }
   void SetStopping( bool value ) { mStopping = value; }

   // Cancel the addition of temporary recording tracks into the project
   void CancelRecording();

   // Audio IO callback methods
   void OnAudioIORate(int rate) override;
   void OnAudioIOStartRecording() override;
   void OnAudioIOStopRecording() override;
   void OnAudioIONewBlocks(const WaveTrackArray *tracks) override;
   void OnCommitRecording() override;
   void OnSoundActivationThreshold() override;

   void OnCheckpointFailure(wxCommandEvent &evt);

   AudacityProject &mProject;

   PlayMode mLastPlayMode{ PlayMode::normalPlay };

   //flag for cancellation of timer record.
   bool mTimerRecordCanceled{ false };

   bool mPaused{ false };
   bool mAppending{ false };
   bool mLooping{ false };
   bool mCutting{ false };
   bool mStopping{ false };

   int mDisplayedRate{ 0 };
   static std::pair< TranslatableStrings, unsigned >
      StatusWidthFunction(
         const AudacityProject &project, StatusBarField field);
};

AUDACITY_DLL_API
AudioIOStartStreamOptions DefaultPlayOptions(
   AudacityProject &project,
   bool newDefault = false /*!< "new" default playback policy adjusts to
      changes of the looping region, "old" default plays once straight */
);
AudioIOStartStreamOptions DefaultSpeedPlayOptions( AudacityProject &project );

struct PropertiesOfSelected
{
   bool allSameRate{ false };
   int rateOfSelected{ RATE_NOT_SELECTED };
   int numberOfSelected{ 0 };
};

AUDACITY_DLL_API
PropertiesOfSelected GetPropertiesOfSelected(const AudacityProject &proj);

#include "commands/CommandFlag.h"

extern AUDACITY_DLL_API const ReservedCommandFlag
   &CanStopAudioStreamFlag();

#endif
