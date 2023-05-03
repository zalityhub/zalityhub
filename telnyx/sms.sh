nbr=$1;shift

curl -X POST \
  --header "Content-Type: application/json" \
  --header "Authorization: Bearer KEY0185D52CEEF26CBA888211D708D87BE3_mVine39rFf32gIzvOr1uz1" \
  --data "{
    \"from\": \"+18084375810\",
    \"to\": \"+1${nbr}\",
    \"text\": \"$*\"
  }" \
  https://api.telnyx.com/v2/messages
