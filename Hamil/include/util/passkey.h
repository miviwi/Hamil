#pragma once

namespace util {

// Helper for using the "Passkey" idiom, which allows declaring methods
//   callable only from specific classes (which avoids making said methods
//   'private' and making the permitted class a friend - which would
//   break encapsulation for all the members)
template <typename PermittedFriend>
class PasskeyFor {
  friend PermittedFriend;

  PasskeyFor() = default;
  PasskeyFor(const PasskeyFor&) = default;
};

}