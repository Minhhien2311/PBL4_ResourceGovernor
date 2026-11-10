using System;
using System.Security.Cryptography;
using System.Text;

namespace WebAdmin_ControlPlane.Utils
{
    public static class HmacHelper
    {
        public static string ComputeHmacSha256(string message, string secret)
        {
            if (string.IsNullOrEmpty(message) || string.IsNullOrEmpty(secret)) return string.Empty;

            var keyBytes = Encoding.UTF8.GetBytes(secret);
            var messageBytes = Encoding.UTF8.GetBytes(message);

            using (var hmac = new HMACSHA256(keyBytes))
            {
                var hashBytes = hmac.ComputeHash(messageBytes);
                // Chuyển đổi sang chuỗi Hex viết thường (VD: a1b2c3d4...)
                return BitConverter.ToString(hashBytes).Replace("-", "").ToLower();
            }
        }
    }
}